#ifndef TWO_H
#define TWO_H

// general library configuration
#include "two-conf.h"

#ifndef TWO_MAX_RESOURCES
#define TWO_MAX_RESOURCES (4)
#endif

#ifdef CONFIG_TWO_MAX_PATH_SIZE
#define TWO_MAX_PATH_SIZE (CONFIG_TWO_MAX_PATH_SIZE)
#else
#define TWO_MAX_PATH_SIZE (32)
#endif

#undef MIN
#define MIN(a, b) ((a) < (b)) ? (a) : (b)

// Defines a resource handler method
//
typedef int (*two_resource_handler_t)(char *method, char *uri, char *response,
                                      unsigned int maxlen);

typedef struct
{
    char path[TWO_MAX_PATH_SIZE];
    char method[8];
    char content_type[32];
    two_resource_handler_t handler;
} two_resource_t;

/*
 * Given a port number, this function start a server
 *
 * @param    port       Port number
 *
 * @return   0          Server was successfully performance
 * @return   -1         An error occurred while the server was running
 */
int two_server_start(unsigned int port);

/**
 * Set callback to handle an http resource
 *
 * A resource is composed by a method and a path
 *
 * A path consists of a sequence of path segments separated by a slash
 * ("/") character.  A path is always defined for a URI, though the
 * defined path may be empty (zero length)
 * (More info in https://tools.ietf.org/html/rfc3986#section-3.3)
 *
 * For this function, the path must start with a '/'.
 *
 * Attempting to define a malformed path, or a path for an unsupported method
 * will result in an error return
 *
 * @param   method          HTTP method for the resource
 * @param   path            Path string, as described above
 * @param   content_type    A IANA valid content type string in the form
 * <type>/<subtype>
 * @param   handler         Callback handler
 *
 * @return  0           if ok
 * @return  -1          if error
 */
int two_register_resource(char *method, char *path, char *content_type,
                          two_resource_handler_t handler);

/**
 * Stop the server as soon as possible
 *
 * This only stops the server from receiving more clients, it does not
 * disconnect already connected clients
 */
void two_server_stop(void (*close_cb)());

#endif /* TWO_H */
