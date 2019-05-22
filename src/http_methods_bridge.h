/*
This API contains the HTTP methods to be used by
HTTP/2
*/
#ifndef HTTP_METHODS_BRIDGE_H
#define HTTP_METHODS_BRIDGE_H

#include <stdlib.h>
#include <stdint.h>
#include "sock.h"
#include "http2.h"

typedef struct TABLE_ENTRY{
  char name [32];
  char value [128];
} table_pair_t;


typedef struct HTTP_STATES{
  uint8_t socket_state;
  sock_t * socket;
  h2states_t h2s;
  uint8_t table_index;
  table_pair_t header_list[10];
} hstates_t;



/*
* Write in the socket with the client
*
* @param   buf   buffer with the data to writte
* @param   len   buffer length
*
* @return >0   number of bytes written
* @return 0    if connection was closed on the other side
* @return -1   on error
*/
int http_write(uint8_t * buf, int len, hstates_t * hs);

/*
* Read the data from the socket with the client
*
* @param    buf   buffer where the data will be write
* @param    len   buffer length
*
* @return   >0     number of bytes read
* @return   0      if connection was closed on the other side
* @return  -1      on error
*/
int http_read(uint8_t * buf, int len, hstates_t * hs);

/*
* Given the content of the request made by the client, this function calls
* the functions necessary to respond to the request
*
* @param    headers   Encoded request
*
* @return   0         the action was successful
* @return   -1        the action fail
*/
int http_receive(char * headers);

#endif /* HTTP_METHODS_H */
