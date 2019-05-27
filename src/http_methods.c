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


<<<<<<< HEAD
struct client_s{
  enum client_state {
      NOT_CLIENT,
      CREATED,
      CONNECTED
  } state;
  sock_t socket;
};

struct server_s{
  enum server_state {
    NOT_SERVER,
    LISTEN,
    CLIENT_CONNECT
  } state;
  sock_t socket;
=======
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
>>>>>>> http_state
};

struct server_s server;
struct client_s client;
hstates_t global_state;


/************************************Server************************************/


int http_init_server(uint16_t port)
{
    global_state.socket_state = 0;
    global_state.table_index = -1;

    client.state = NOT_CLIENT;

  if (sock_create(&server.socket)<0){
    ERROR("Error in server creation");
    return -1;
  }

  if (sock_listen(&server.socket, port)<0){
    ERROR("Partial error in server creation");
    return -1;
  }

<<<<<<< HEAD
  server.state=LISTEN;
=======
    server.state = LISTEN;
>>>>>>> http_state

  printf("Server waiting for a client\n");


  if (sock_accept(&server.socket, &client.socket)<0){
    ERROR("Not client found");
    return -1;
  }

  client.state=CONNECTED;

    printf("Client found and connected\n");

    global_state.socket = &client.socket;
    global_state.socket_state = 1;
    if (server_init_connection(&global_state) < 0) {
        ERROR("Problems sending server data");
        return -1;
    }

    return 0;
}

int http_set_function_to_path(char *callback, char *path)
{
    (void)callback;
    (void)path;
    //TODO callback(argc,argv)
    return -1;
}


int http_add_header(char *name, char *value)
{
    int i = global_state.table_index;

<<<<<<< HEAD
  if (server.state==NOT_SERVER){
    WARN("Server not found");
    return -1;
  }

  if (client.state==CONNECTED){
    if (sock_destroy(&client.socket)<0){
      WARN("Client still connected");
=======
    if (i == 10) {
        ERROR("Headers list is full");
        return -1;
>>>>>>> http_state
    }

<<<<<<< HEAD
  DEBUG("HERE");
  if (sock_destroy(&server.socket)<0){
    ERROR("Error in server disconnection");
    return -1;
  }
  DEBUG("AFTER DESTROY");
=======
    strcpy(global_state.header_list[i].name, name);
    strcpy(global_state.header_list[i].value, value);
>>>>>>> http_state

    global_state.table_index = i + 1;

    return 0;
}


<<<<<<< HEAD
/************************************Client************************************/


int http_client_connect(uint16_t port, char * ip){
  struct client_s * cl= &client;
  server.state=NOT_SERVER;

  if (sock_create(&cl->socket)<0){
    ERROR("Error on client creation");
    return -1;
  }
=======
int http_server_destroy(void)
{
    if (server.state == NOT_SERVER) {
        WARN("Server not found");
        return -1;
    }
>>>>>>> http_state

    if (client.state == CONNECTED || global_state.socket_state == 1) {
        if (sock_destroy(&client.socket) < 0) {
            WARN("Client still connected");
        }
    }

<<<<<<< HEAD
  if (sock_connect(&cl->socket, ip, port)<0){
    ERROR("Error on client connection");
    return -1;
  }
=======
    global_state.socket_state = 0;
>>>>>>> http_state

    if (sock_destroy(&server.socket) < 0) {
        ERROR("Error in server disconnection");
        return -1;
    }

    server.state = NOT_SERVER;

    printf("Server destroyed\n");

    return 0;
}


/************************************Client************************************/


<<<<<<< HEAD
  if (sock_destroy(&client.socket)<0){
    ERROR("Error in client disconnection");
    return -1;
  }
=======
int http_client_connect(uint16_t port, char *ip)
{
    global_state.socket_state = 0;
    global_state.table_index = -1;
>>>>>>> http_state

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

    global_state.socket = &cl->socket;
    global_state.socket_state = 1;

    if (client_init_connection(&global_state) < 0) {
        ERROR("Problems sending client data");
        return -1;
    }

    return 0;
}


char *http_grab_header(char *header, int len)
{
    int i = global_state.table_index;

<<<<<<< HEAD
  if (client.state == CONNECTED){
    wr= sock_write(&client.socket, (char *) buf, len);
  } else {
    ERROR("No client connected found");
    return -1;
  }
=======
    if (i == -1) {
        ERROR("Headers list is empty");
        return NULL;
    }
>>>>>>> http_state

    int k;
    for (k = 0; k <= i; k++) {
        if (strncmp(global_state.header_list[k].name, header, len)==0){
          return global_state.header_list[k].value;
        }
    }
    return 0;
}


int http_client_disconnect(void)
{

<<<<<<< HEAD
  if (client.state == CONNECTED){
    rd= sock_read(&client.socket, (char *) buf, len, 0);
  } else {
    ERROR("No client connected found");
    return -1;
  }
=======
    if (client.state == CONNECTED || global_state.socket_state == 1) {
        if (sock_destroy(&client.socket) < 0) {
            ERROR("Error in client disconnection");
            return -1;
        }
>>>>>>> http_state

        printf("Client disconnected\n");
    }

    global_state.socket_state = 0;
    client.state = NOT_CLIENT;

    return 0;
}
