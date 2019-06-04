/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http_methods.h"
#include "http_methods_bridge.h"
#include "logging.h"
#include "sock.h"
#include "http2.h"


struct client_s {
    enum client_state {
        NOT_CLIENT,
        CREATED,
        CONNECTED
    } state;
    sock_t socket;
};

struct server_s {
    enum server_state {
        NOT_SERVER,
        LISTEN,
        CLIENT_CONNECT
    } state;
    sock_t socket;
};

struct server_s server;
struct client_s client;

/************************************Server************************************/


int http_init_server(hstates_t *hs, uint16_t port)
{
    hs->socket_state = 0;
    hs->table_count = 0;
    hs->connection_state = 0;

    client.state = NOT_CLIENT;

    if (sock_create(&server.socket) < 0) {
        ERROR("Error in server creation");
        return -1;
    }

    if (sock_listen(&server.socket, port) < 0) {
        ERROR("Partial error in server creation");
        return -1;
    }

    server.state = LISTEN;

    printf("Server waiting for a client\n");


    while (sock_accept(&server.socket, &client.socket) >= 0) {
        client.state = CONNECTED;

        printf("Client found and connected\n");

        hs->socket = &client.socket;
        hs->socket_state = 1;
        hs->connection_state = 1;

        if (h2_server_init_connection(hs) < 0) {
            ERROR("Problems sending server data");
            return -1;
        }

        while (hs->connection_state != 1) {
            if (h2_receive_frame(hs) < 1) {
              break;
            }
        }

        hs->connection_state=0;
        hs->socket_state=0;
        if (sock_destroy(hs->socket)==-1){
          WARN("Could not destroy client socket");
        }
    }

    ERROR("Not client found");

    return -1;
}

int http_set_function_to_path(char *callback, char *path)
{
    (void)callback;
    (void)path;
    //TODO callback(argc,argv)
    return -1;
}


int http_set_header(hstates_t *hs, char *name, char *value)
{
    int i = hs->table_count;

    if (i == HTTP2_MAX_HEADER_COUNT) {
        ERROR("Headers list is full");
        return -1;
    }

    strcpy(hs->header_list[i].name, name);
    strcpy(hs->header_list[i].value, value);

    hs->table_count = i + 1;

    return 0;
}


int http_server_destroy(hstates_t *hs)
{
    if (server.state == NOT_SERVER) {
        WARN("Server not found");
        return -1;
    }

    if (client.state == CONNECTED || hs->socket_state == 1) {
        if (sock_destroy(&client.socket) < 0) {
            WARN("Client still connected");
        }
    }

    hs->socket_state = 0;
    hs->connection_state = 0;

    if (sock_destroy(&server.socket) < 0) {
        ERROR("Error in server disconnection");
        return -1;
    }

    server.state = NOT_SERVER;

    printf("Server destroyed\n");

    return 0;
}


/************************************Client************************************/


int http_client_connect(hstates_t * hs, uint16_t port, char *ip)
{
    hs->socket_state = 0;
    hs->table_count = 0;
    hs->connection_state = 0;

    struct client_s *cl = &client;
    server.state = NOT_SERVER;

    if (sock_create(&cl->socket) < 0) {
        ERROR("Error on client creation");
        return -1;
    }

    cl->state = CREATED;

    if (sock_connect(&cl->socket, ip, port) < 0) {
        ERROR("Error on client connection");
        return -1;
    }

    printf("Client connected to server\n");

    cl->state = CONNECTED;

    hs->socket = &cl->socket;
    hs->socket_state = 1;
    hs->connection_state = 1;

    if (h2_client_init_connection(hs) < 0) {
        ERROR("Problems sending client data");
        return -1;
    }

    return 0;
}


char *http_get_header(hstates_t *hs, char *header)
{
    int i = hs->table_count;

    if (i == 0) {
        WARN("Headers list is empty");
        return NULL;
    }

    int k;
    for (k = 0; k <= i; k++) {
        if (strncmp(hs->header_list[k].name, header, strlen(header)) == 0) {
            INFO("RETURNING value of '%s' header; '%s'", hs->header_list[k].name, hs->header_list[k].value);
            return hs->header_list[k].value;
        }
    }

    WARN("Header not found in headers list");
    return NULL;
}


int http_client_disconnect(hstates_t *hs)
{
    if (client.state == CONNECTED || hs->socket_state == 1) {
        if (sock_destroy(&client.socket) < 0) {
            ERROR("Error in client disconnection");
            return -1;
        }

        printf("Client disconnected\n");
    }

    hs->socket_state = 0;
    hs->connection_state = 0;
    client.state = NOT_CLIENT;

    return 0;
}
