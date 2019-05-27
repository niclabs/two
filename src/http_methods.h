/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */
#ifndef HTTP_METHODS_H
#define HTTP_METHODS_H

#include "http_methods_bridge.h"



/*Server*/
/*
 * Given a port number this function initialize a server
 *
 * @param    port    port number
 *
 * @return   0       Server was successfully initialized
 * @return   -1      Server wasn't initialized
 */
int http_init_server(uint16_t port);


/*
 * Stop and destroy server if it is running
 *
 * @return   0       Server was successfully destroyed
 * @return   -1      Server wasn't destroyed
 */
int http_server_destroy(void);


/*
 * Set an internal server function to a specific path
 *
 * @param    callback   function name
 * @param    path       specific path
 *
 * @return   0          the action was successful
 * @return   -1         the action fail
 */
int http_set_function_to_path(char *callback, char *path);


/*Client*/

/*
 * Start a connection from client to server
 *
 * @param    port   port number
 * @param    ip     adress
 *
 * @return   0      successfully started connection
 * @return   -1     the connection fail
 */
int http_client_connect(uint16_t port, char *ip);

/*
 * Stop the connection between client and server
 *
 * @return    0    connection was stopped
 * @return    -1   failed to stop connection
 */
int http_client_disconnect(void);

/*
 * Add a header and its value to the headers list
 *
 * @param    name      new headers name
 * @param    value     new headers value
 *
 * @return   0      successfully added pair
 * @return   -1     There was an error
 */
int http_add_header(char *name, char *value);

/*
 * Search by a value of a header in the header list
 *
 * @param    header     Header name
 * @param    len        header name length
 *
 * @return              value finded
 */
char *http_grab_header(char *header, int len);


#endif /* HTTP_METHODS_H */
