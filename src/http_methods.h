/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <stdlib.h>

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
* Write in the socket with the client
*
* @param   buf   buffer with the data to writte
* @param   len   buffer length
*
* @return >0   number of bytes written
* @return 0    if connection was closed on the other side
* @return -1   on error
*/
int http_write(uint8_t * buf, int len);

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
int http_read(uint8_t * buf, int len);


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
int http_client_connect(uint16_t port, char * ip);

/*
* Stop the connection between client and server
*
* @return    0    connection was stopped
* @return    -1   failed to stop connection
*/
int http_client_disconnect(void);
