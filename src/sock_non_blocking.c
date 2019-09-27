#include "sock_non_blocking.h"

struct sock_sock_struct {};

int sock_create(sock_t* socket)
{
    return 0;
}

int sock_listen(sock_t* server_socket, uint16_t port)
{
    return 0;
}

int sock_poll(sock_t* socket)
{
    return 0;
}

int sock_read(sock_t* socket, char * buffer, unsigned int buffer_length)
{
    return 0;
}

int sock_write(sock_t* socket, char * buffer, unsigned int buffer_length)
{
    return 0;
}

int sock_accept(sock_t* server_socket, sock_t* client_socket)
{
    return 0;
}