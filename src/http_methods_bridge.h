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

typedef struct TABLE_ENTRY {
    char name [32];
    char value [128];
} table_pair_t;

/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    uint8_t wait_setting_ack;

} h2states_t;

typedef struct HTTP_STATES {
    uint8_t socket_state;
    sock_t *socket;
    h2states_t h2s;
    uint8_t table_index;
    table_pair_t header_list[HTTP2_MAX_HEADER_COUNT];
    uint8_t header_count;
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
 * @param    index     header index in headers list, invalid index to delete
 *                     the entire table
 *
 * @return    0        The list was emptied
 * @return    1        There was an error
 */
int http_clear_header_list(hstates_t *hs, int index);

#endif /* HTTP_METHODS_H */
