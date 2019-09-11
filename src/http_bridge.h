/*
   This API contains the internal methods in HTTP layer to be used by HTTP/2
   and HTTP
 */
#ifndef HTTP_BRIDGE_H
#define HTTP_BRIDGE_H


#include <stdlib.h>
#include <stdint.h>
#include "sock.h"
#include "headers.h"
#include "hpack.h"



#define HTTP2_MAX_HBF_BUFFER 16384

typedef enum {
    STREAM_IDLE,
    STREAM_OPEN,
    STREAM_HALF_CLOSED_REMOTE,
    STREAM_HALF_CLOSED_LOCAL,
    STREAM_CLOSED
} h2_stream_state_t;

typedef struct HTTP2_STREAM {
    uint32_t stream_id;
    h2_stream_state_t state;
} h2_stream_t;

typedef struct HTTP2_WINDOW_MANAGER {
    uint32_t window_size;
    uint32_t window_used;
} h2_window_manager_t;



/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    uint8_t wait_setting_ack;
    h2_stream_t current_stream;
    uint32_t last_open_stream_id;
    uint8_t header_block_fragments[HTTP2_MAX_HBF_BUFFER];
    uint8_t header_block_fragments_pointer; //points to the next byte to write in
    uint8_t waiting_for_end_headers_flag;   //bool
    uint8_t received_end_stream;
    h2_window_manager_t incoming_window;
    h2_window_manager_t outgoing_window;
    uint8_t sent_goaway;
    uint8_t received_goaway;        // bool
    uint8_t debug_data_buffer[0];   // TODO not implemented yet
    uint8_t debug_size;             // TODO not implemented yet
    //Hpack dynamic table
    hpack_states_t hpack_states;
} h2states_t;


/*---------------------HTTP structures and static values--------------------*/

#define HTTP_MAX_CALLBACK_LIST_ENTRY 32

#ifdef HTTP_CONF_MAX_RESOURCES
#define HTTP_MAX_RESOURCES (HTTP_CONF_MAX_RESOURCES)
#else
#define HTTP_MAX_RESOURCES (16)
#endif

#ifdef HTTP_CONF_MAX_HOST_SIZE
#define HTTP_MAX_HOST_SIZE (HTTP_CONF_MAX_HOST_SIZE)
#else
#define HTTP_MAX_HOST_SIZE (64)
#endif


#ifdef HTTP_CONF_MAX_PATH_SIZE
#define HTTP_MAX_PATH_SIZE (HTTP_CONF_MAX_PATH_SIZE)
#else
#define HTTP_MAX_PATH_SIZE (32)
#endif

#ifdef HTTP_CONF_MAX_RESPONSE_SIZE
#define HTTP_MAX_RESPONSE_SIZE (HTTP_CONF_MAX_RESPONSE_SIZE)
#else
#define HTTP_MAX_RESPONSE_SIZE (128)
#endif

#ifdef HTTP_CONF_MAX_HEADER_COUNT
#define HTTP_MAX_HEADER_COUNT (HTTP_CONF_MAX_HEADER_COUNT)
#else
#define HTTP_MAX_HEADER_COUNT (16)
#endif

#ifdef HTTP_CONF_MAX_DATA_SIZE
#define HTTP_MAX_DATA_SIZE (HTTP_CONF_MAX_DATA_SIZE)
#else
#define HTTP_MAX_DATA_SIZE (16384)
#endif

/**
 * Definition of a resource handler, i.e. an action to perform on call of a given
 * method and uri
 *
 * the following params are given to the handler
 *
 * @param method    HTTP method that triggered the request. There is not need to check for
 *                  method support, the server only will call the method on reception of a
 *                  matching method and path as registered with http_register_resource_handler()
 * @param uri       HTTP uri including path and query params that triggered the request
 * @param response  pointer to an uint8 array to store the response
 * @param maxlen    maximum length of the response
 *
 * the handler must return the number of bytes written or -1 if an error ocurred
 */
typedef int (*http_resource_handler_t) (char *method, char *uri, uint8_t *response, int maxlen);

typedef struct {
    char path[HTTP_MAX_PATH_SIZE];
    char method[8];
    http_resource_handler_t handler;
} http_resource_t;

typedef struct {
    uint32_t size;
    uint8_t buf[HTTP_MAX_DATA_SIZE];
    uint32_t processed;
} http_data_t;

typedef struct HTTP_STATES {
    char host[HTTP_MAX_HOST_SIZE];

    uint8_t connection_state;
    uint8_t socket_state;
    sock_t socket; // client socket
    uint8_t server_socket_state;
    sock_t server_socket;
    uint8_t is_server; // boolean flag to know if current hstates if server or client
    h2states_t h2s;
    http_data_t data_in;
    http_data_t data_out;

    // Http headers storage
    // TODO: deprecate and pass directly to http2 functions
    headers_t headers_in;
    headers_t headers_out;

    // Resource handler list
    http_resource_t resource_list[HTTP_MAX_RESOURCES];
    uint8_t resource_list_size;

    //boolean. Notifies HTTP if new headers were written
    uint8_t new_headers;
    //boolean. 0 = full message not yet received 1 = full message received
    uint8_t end_message;
    //boolean. 0 = END STREAM, 1 = keep_receiving
    uint8_t keep_receiving;
    //boolean. 0 = received goaway without error, 1 = received goaway with error
    uint8_t evil_goodbye;

} hstates_t;

/*--------------------------------------------------------------------------*/


/*
 * Write in the socket with the client
 *
 * @param    hs        Struct with server/client information
 * @param    buf       Buffer with the data to writte
 * @param    len       Buffer length
 *
 * @return   >0        Number of bytes written
 * @return   0         If connection was closed on the other side
 * @return   -1        On error
 */
int http_write(hstates_t *hs, uint8_t *buf, int len);


/*
 * Read the data from the socket with the client
 *
 * @param    hs        Struct with server/client information
 * @param    buf       Buffer where the data will be write
 * @param    len       Buffer length
 *
 * @return   >0        Number of bytes read
 * @return   0         If connection was closed on the other side
 * @return   -1        On error
 */
int http_read(hstates_t *hs, uint8_t *buf, int len);


/*
 * Empty the list of headers in hstates_t struct
 *
 * @param    hs        Struct with server/client and headers information
 * @param    index     Header index in the headers list to be maintained,
 *                     invalid index to delete the entire table
 *
 * @return    0        The list was emptied
 * @return    1        There was an error
 */
int http_clear_header_list
    (hstates_t *hs, int index, int flag);


#endif /* HTTP_BRIDGE_H */
