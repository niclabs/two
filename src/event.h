/**
 * Manage generic I/O events
 *
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef EVENT_H
#define EVENT_H

/* Event interactions */
typedef int (*handle_getter_t)(void *instance);
typedef void (*event_handler_t)(void *instance);


typedef struct {
    void *instance;
    handle_getter_t get_handle;
    event_handler_t handle_event;
} event_t;


int register_event(event_t *event);
int unregister_event(event_t *event);

#endif
