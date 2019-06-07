/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */
#ifndef HTTP_H
#define HTTP_H

#include "http_bridge.h"



/*Server*/
/*
 * Given a port number this function initialize a server
 *
 * @param    hs      struct for server information
 * @param    port    port number
 *
 * @return   0       Server was successfully initialized
 * @return   -1      Server wasn't initialized
 */
int http_init_server(hstates_t *hs, uint16_t port);


/*
 * Set an internal server function to a specific path
 *
 * @param    callback   function name
 * @param    path       specific path
 *
 * @return   0          the action was successful
 * @return   -1         the action fail
 */
int http_set_function_to_path(hstates_t *hs, char *callback, char *path);


/*
 * Add a header and its value to the headers list
 *
 * @param    hs        struct with headers information
 * @param    name      new headers name
 * @param    value     new headers value
 *
 * @return   0      successfully added pair
 * @return   -1     There was an error
 */
int http_set_header(hstates_t *hs, char *name, char *value);


/*
 * Stop and destroy server if it is running
 *
 * @param    hs      struct with server information
 *
 * @return   0       Server was successfully destroyed
 * @return   -1      Server wasn't destroyed
 */
int http_server_destroy(hstates_t *hs);



/*Client*/

/*
 * Start a connection from client to server
 *
 * @param    hs     struct for client information
 * @param    port   port number
 * @param    ip     adress
 *
 * @return   0      successfully started connection
 * @return   -1     the connection fail
 */
int http_client_connect(hstates_t *hs, uint16_t port, char *ip);


/*
 * Search by a value of a header in the header list
 *
 * @param    hs         struct with headers information
 * @param    header     Header name
 *
 * @return              value finded
 */
char *http_get_header(hstates_t *hs, char *header);


/*
 * Stop the connection between client and server
 *
 * @param    hs     struct with client information
 *
 * @return    0    connection was stopped
 * @return    -1   failed to stop connection
 */
int http_client_disconnect(hstates_t *hs);


#endif /* HTTP_H */
