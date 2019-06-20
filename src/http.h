/*
   This API contains the methods in HTTP layer
*/
#ifndef HTTP_H
#define HTTP_H

#include "http_bridge.h"

typedef struct CALLBACK_TYPE_S {
    int (*cb)(headers_lists_t *);
} callback_type_t;

/*Server*/
/*
 * Given a port number and a struct for server information, this function
 * initialize a server
 *
 * @param    hs         Struct for server information
 * @param    port       Port number
 *
 * @return   0          Server was successfully initialized
 * @return   -1         Server wasn't initialized
 */
int http_init_server(hstates_t *hs, uint16_t port);


/*
 * Given a struct with server information, this function starts the server
 *
 * @param    hs         Struct with server information
 *
 * @return   0          Server was successfully started
 * @return   -1         There was an error in the process
 */
int http_start_server(hstates_t *hs);


/*
 * Stop and destroy server if it is running
 *
 * @param    hs         Struct with server information
 *
 * @return   0          Server was successfully destroyed
 * @return   -1         Server wasn't destroyed
 */
int http_server_destroy(hstates_t *hs);


/*
 * Set an internal server function to a specific path
 *
 * @param    hs         Struct with server information
 * @param    callback   Function name
 * @param    path       Specific path
 *
 * @return   0          The action was successful
 * @return   -1         The action fail
 */
int http_set_function_to_path(hstates_t *hs, callback_type_t callback, char *path);



/*Client*/

/*
 * Start a connection from client to server
 *
 * @param    hs         Struct with client information
 * @param    port       Port number
 * @param    ip         Server IP address
 *
 * @return   0          Successfully started connection
 * @return   -1         The connection fail
 */
int http_client_connect(hstates_t *hs, uint16_t port, char *ip);


int http_get(hstates_t *hs, char *path, char *accept_type);


/*
 * Stop the connection between client and server
 *
 * @param    hs         Struct with client information
 *
 * @return    0         Connection was stopped
 * @return    -1        Failed to stop connection
 */
int http_client_disconnect(hstates_t *hs);



/*Headers*/
/*
 * Add a header and its value to the headers list
 *
 * @param    hs         Struct with server/client and headers information
 * @param    name       New headers name
 * @param    value      New headers value
 *
 * @return   0          Successfully added pair
 * @return   -1         There was an error in the process
 */
int http_set_header(headers_lists_t *h_lists, char *name, char *value);


/*
 * Search by a value of a header in the header list
 *
 * @param    hs         Struct with server/client and headers information
 * @param    header     Header name
 *
 * @return              Value finded
 */
char *http_get_header(headers_lists_t *h_lists, char *header);


int http_start_client(hstates_t* hs);

#endif /* HTTP_H */
