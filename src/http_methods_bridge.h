/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */
#ifndef HTTP_METHODS_BRIDGE_H
#define HTTP_METHODS_BRIDGE_H

#include <stdlib.h>
#include <stdint.h>
#include "sock.h"

#define HTTP2_MAX_HEADER_COUNT 32

/*-----HTTP2 structures-----*/

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

/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    uint8_t wait_setting_ack;
    h2_stream_t current_stream;
} h2states_t;

/*-----HTTP structures-----*/


typedef struct TABLE_ENTRY {
    char name [32];
    char value [128];
} table_pair_t;

typedef struct HTTP_STATES {
    uint8_t conection_state;
    uint8_t socket_state;
    sock_t *socket;
    h2states_t h2s;
    uint8_t table_count;
    table_pair_t header_list[HTTP2_MAX_HEADER_COUNT];
    uint8_t header_count;
    uint8_t header_block_fragments[128];
    uint8_t header_block_fragments_pointer; //points to the next byte to write in
    uint8_t waiting_for_end_headers_flag;   //bool
} hstates_t;




/*
 * Write in the socket with the client
 *
 * @param    buf   buffer with the data to writte
 * @param    len   buffer length
 * @param    hs    http states struct
 *
 * @return >0   number of bytes written
 * @return 0    if connection was closed on the other side
 * @return -1   on error
 */
int http_write(uint8_t *buf, int len, hstates_t *hs);

/*
 * Read the data from the socket with the client
 *
 * @param    buf   buffer where the data will be write
 * @param    len   buffer length
 * @param    hs    http states struct
 *
 * @return   >0    number of bytes read
 * @return   0     if connection was closed on the other side
 * @return   -1    on error
 */
int http_read(uint8_t *buf, int len, hstates_t *hs);

/*
 * Given the content of the request made by the client, this function calls
 * the functions necessary to respond to the request
 *
 * @param     headers   Encoded request
 *
 * @return    0         the action was successful
 * @return    -1        the action fail
 */
int http_receive(char *headers);

/*
 * Empty the list of headers in hstates_t struct
 *
 * @param    hs        http states struct
 * @param    index     header index in the headers list to be maintained,
 *                     invalid index to delete the entire table
 *
 * @return    0        The list was emptied
 * @return    1        There was an error
 */
int http_clear_header_list(hstates_t *hs, int index);

#endif /* HTTP_METHODS_H */
