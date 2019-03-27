/**
 * HTTP/2 Server API
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef SERVER_H
#define SERVER_H

#include <stdlib.h>
#include <sys/types.h>

/* ADT definition of server */
typedef struct server server_t;

server_t * server_create(uint16_t port);
void server_listen(server_t * server);
void server_destroy(server_t * server);


#endif /* SERVER_H */