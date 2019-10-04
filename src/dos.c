/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#include "dos.h"
#include "logging.h"

int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return res_manager_server_register_resource(method, path, handler);
}
