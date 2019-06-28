/*
   This API contains the methods in HTTP layer
*/
#ifndef HTTP_H
#define HTTP_H

#include "http_bridge.h"

typedef struct CALLBACK_TYPE_S {
    int (*cb)(headers_data_lists_t *);
} callback_type_t;

typedef struct RESPONSE_RECEIVED_TYPE_S {
    int size_data;
    char *status_flag;
    uint8_t *data;
}response_received_type_t;


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
 * Initialize a connection from client to server
 *
 * @param    hs         Struct with client information
 * @param    port       Port number
 * @param    ip         Server IP address
 *
 * @return   0          Successfully started connection
 * @return   -1         The connection fail
 */
int http_client_connect(hstates_t *hs, uint16_t port, char *ip);


/*
 * Wait for data from server
 *
 * @param    hs         Struct with client information
 *
 * @return   0          Successfully started connection
 * @return   -1         The connection fail
 */
int http_start_client(hstates_t* hs);


/*
 * Send a GET request to server and wait for an answer
 *
 * @param    hs             Struct with client information
 * @param    path           Path where looking for answer
 * @param    host           host for query
 * @param    accept_type    Type accepted for answer
 *
 * @return   0          The action was successful
 * @return   -1         The action failed
 */
int http_get(hstates_t *hs, char *path, char *host, char *accept_type, response_received_type_t *rr);


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
 * @param    hd_lists         Struct with headers information
 * @param    name             New headers name
 * @param    value            New headers value
 *
 * @return   0                Successfully added pair
 * @return   -1               There was an error in the process
 */
int http_set_header(headers_data_lists_t *hd_lists, char *name, char *value);


/*
 * Search by a value of a header in the header list
 *
 * @param    hd_lists         Struct with headers information
 * @param    header           Header name
 *
 * @return                    Value finded
 */
char *http_get_header(headers_data_lists_t *hd_lists, char *header);


/*
 * Add data to be sent to data lists
 *
 * @param    hd_lists   Struct with data information
 * @param    data       Data
 *
 * @return   0          Successfully added data
 * @return   -1         There was an error in the process
 */
int http_set_data(headers_data_lists_t *hd_lists, uint8_t *data);


/*
 * Returns received data
 *
 * @param    hd_lists         Struct with data information
 * @param    data_size        Header name
 *
 * @return                    Value finded
 */
uint8_t *http_get_data(headers_data_lists_t *hd_lists, int *data_size);


#endif /* HTTP_H */
