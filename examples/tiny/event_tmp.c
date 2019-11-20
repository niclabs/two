#include <errno.h>
#include <assert.h>

#include "contiki.h"

#include "event.h"

#define LOG_MODULE EVENT
#define LOG_LEVEL_EVENT LOG_LEVEL_DEBUG
#include "logging.h"

PROCESS(event_loop_process, "Event loop process");

// private methods

void event_loop_sock(void *state)
{
    event_sock_t *s = (event_sock_t *)state;

    if (uip_connected()) {
        if (s == NULL) { // new client connection
            // todo: lookup in the polling list for a client for port
        }
    }
}

int event_loop_is_alive(event_loop_t *loop)
{
    return loop->polling != NULL;
}

void event_loop_init(event_loop_t *loop)
{
    // fail if loop pointer is not initialized
    assert(loop != NULL);

    // reset loop memory
    memset(loop, 0, sizeof(event_loop_t));

    // create unused socket list
    for (int i = 0; i < EVENT_MAX_HANDLES - 1; i++) {
        loop->sockets[i].next = &loop->sockets[i + 1];
    }

    // list of unused sockets
    loop->unused = &loop->sockets[0];
}


void event_loop(event_loop_t *loop)
{
    assert(loop != NULL);
    assert(loop->running == 0);

    // start loop
    process_start(&event_loop_process, loop);
}

PROCESS_THREAD(event_loop_process, ev, data){
    PROCESS_BEGIN();

    // on first call data will be the event loop instance
    static event_loop_t *loop = (event_loop_t * loop)data;
    loop->running = 1;

    while (event_loop_is_alive(loop)) {
        PROCESS_WAIT_EVENT();
        // perform timer events

        // perform I/O events

        if (ev == tcpip_event) {
            event_loop_poll(data);
        }

        // perform close events
        event_loop_close(loop);
    }

    // end loop
    loop->running = 0;
    PROCESS_END();
}
