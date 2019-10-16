/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#include "dos.h"
#include "http2/http2_v2.h"
#include "http2/http2_structs.h"
#include "net.h"
#include "logging.h"


int two_server_start(unsigned int port)
{
    callback_t default_callback;
    int stop_flag = 0;
    size_t data_buffer_size = 1024*4;
    size_t client_state_size = sizeof(h2states_t);

    int status = (int) net_server_loop(port, default_callback, &stop_flag, data_buffer_size, client_state_size);

    return status;
}


int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return resource_handler_set(method, path, handler);
}
