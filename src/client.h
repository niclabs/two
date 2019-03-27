/**
 * HTTP/2 Client API
 * 
 * @author Felipe Lalanne <flalanne@niclabs.cl>
 */
#ifndef CLIENT_H
#define CLIENT_H

#include <stdlib.h>

/* ADT definition of server */
typedef struct client client_t;

client_t *client_create(char *addr, uint16_t port);
void client_connect(client_t *client);
void client_request(client_t *client, char *endpoint);
void client_destroy(client_t *client);

#endif /* CLIENT_H */
