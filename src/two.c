/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include "two.h"
#include "event.h"
#include "http2/http2.h"
#include "http2/structs.h"
#include "logging.h"

#ifndef TWO_MAX_CLIENTS
#define TWO_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)
#endif

// static variables
static event_loop_t loop;
static event_sock_t *server;

static h2states_t http2_client_list[TWO_MAX_CLIENTS];

int two_server_start(unsigned int port)
{
    memset(http2_client_list, 0, TWO_MAX_CLIENTS * sizeof(h2states_t));
    
    event_loop_init(&loop);
    server = event_sock_create(&loop);

    int r = event_listen(server, port, http2_server_init_connection);

    INFO("Starting http/2 server in port 8888");
    if (r < 0) {
        ERROR("Could not start server");
        return 1;
    }
    event_loop(&loop);

    return 0;
}


int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return resource_handler_set(method, path, handler);
}
