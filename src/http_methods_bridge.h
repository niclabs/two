/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <stdlib.h>
#include <stdint.h>
#include "http2.h"

struct HTTP_STATES{
  uint8_t socket_state;
  sock_t * socket;
  h2states_t h2s;
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
int http_write(uint8_t * buf, int len, hstates_t hs);

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
int http_read(uint8_t * buf, int len, hstates_t hs);
