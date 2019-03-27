/**
 * API for managing generic I/O events
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef EVENT_H
#define EVENT_H

/* Event interactions */
typedef int (*get_fd_cb_t)(void *instance);
typedef void (*handle_event_cb_t)(void *instance);


typedef struct {
    void *instance;
    get_fd_cb_t get_fd;
    handle_event_cb_t handle;
} event_handler_t;


int register_handler(event_handler_t *event);
int unregister_handler(event_handler_t *event);

#endif
