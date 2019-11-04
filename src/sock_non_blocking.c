#include "sock_non_blocking.h"

struct sock_sock_struct {};

int sock_create(sock_t* socket)
{
    (void)socket;
    return 0;
}

int sock_destroy(sock_t* socket)
{
    (void)socket;
    return 0;
}

unsigned int sock_poll(sock_t* socket)
{
    (void)socket;
    return 0;
}

int sock_read(sock_t* socket, uint8_t * buffer, unsigned int buffer_length)
{
    (void)socket;
    (void)buffer;
    (void)buffer_length;
    return 0;
}

int sock_write(sock_t* socket, uint8_t * buffer, unsigned int buffer_length)
{
    (void)socket;
    (void)buffer;
    (void)buffer_length;
    return 0;
}

int sock_listen(sock_t* server_socket, uint16_t port)
{
    (void)server_socket;
    (void)port;
    return 0;
}

int sock_accept(sock_t* server_socket, sock_t* client_socket)
{
    (void)server_socket;
    (void)client_socket;
    return 0;
}