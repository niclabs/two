/**
 * Implementation of reactor pattern with select() calls
 * as an event-loop
 *
 * Based on https://github.com/adamtornhill/PatternsInC/tree/master/5_Reactor
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <sys/select.h>

#include "event_loop.h"
#include "logging.h"

#define MAX_NO_OF_HANDLES 32

// Active file descriptors
static fd_set active_fd_set;
static int active_fd_set_is_init = 0;
static int max_fd = -1;

/**
 * List item definition for a list of
 * handlers
 */
typedef struct {
    int fd;
    int is_used;
    event_handler_t *event;
} handler_list_entry_t;

// List of registered handlers
static handler_list_entry_t registered_handlers_list[MAX_NO_OF_HANDLES];

event_handler_t *find_handler(int fd)
{
    event_handler_t *match = NULL;

    for (int i = 0; i < MAX_NO_OF_HANDLES && match == NULL; i++) {
        if (registered_handlers_list[i].is_used && registered_handlers_list[i].fd == fd) {
            match = registered_handlers_list[i].event;
        }
    }
    return match;
}

int find_free_handler()
{
    int slot = -1;
    for (int i = 0; i < MAX_NO_OF_HANDLES && slot < 0; i++) {
        if (registered_handlers_list[i].is_used == 0) {
            slot = i;
        }
    }
    return slot;
}

int register_handler(event_handler_t *event)
{
    assert(NULL != event);

    // First call
    if (active_fd_set_is_init == 0) {
        FD_ZERO(&active_fd_set); // Initialize the set of active handles
        active_fd_set_is_init = 1;
    }

    int fd = event->get_fd(event->instance);
    if (!FD_ISSET(fd, &active_fd_set)) {
        // Look for a free entry in the registered event array
        int free_slot = find_free_handler();
        if (free_slot >= 0) {
            handler_list_entry_t *free_entry = &registered_handlers_list[free_slot];
            free_entry->event = event;
            free_entry->fd = fd;
            free_entry->is_used = 1;

            FD_SET(fd, &active_fd_set);
            if (fd > max_fd) {
                max_fd = fd;
            }

            return 0;
        }
    }

    return -1;
}

int unregister_handler(event_handler_t *event)
{
    assert(NULL != event);

    // First call
    if (active_fd_set_is_init < 0) {
        FD_ZERO(&active_fd_set);    // Initialize the set of active handles
        active_fd_set_is_init = 1;  // And reset counter
    }

    int fd = event->get_fd(event->instance);
    int node_removed = -1;
    if (FD_ISSET(fd, &active_fd_set)) {
        for (int i = 0; (i < MAX_NO_OF_HANDLES) && (node_removed < 0); i++) {
            if (registered_handlers_list[i].is_used && registered_handlers_list[i].fd == fd) {
                registered_handlers_list[i].is_used = 0;
                node_removed = 0;

                // Clear the file descriptor and find the new max_fd
                FD_CLR(fd, &active_fd_set);
                while (FD_ISSET(max_fd, &active_fd_set)) max_fd--;
            }
        }
    }
    return node_removed;
}

void handle_events(void)
{
    fd_set read_fd_set;

    for (;;) {
        /* Block until input arrives on one or more active sockets. */
        read_fd_set = active_fd_set;
        if (select(max_fd + 1, &read_fd_set, NULL, NULL, NULL) < 0) {
            ERROR("In select(): %s\n", strerror(errno));
            break;
        }

        /* Service all the sockets with input pending. */
        for (int i = 0; i <= max_fd; ++i) {
            if (FD_ISSET(i, &read_fd_set)) {
                event_handler_t *signalled_event = find_handler(i);
                if (signalled_event != NULL) {
                    signalled_event->handle(signalled_event->instance);
                }
            }
        }
    }
}
