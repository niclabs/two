/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#include <unistd.h>

#include "http_methods.h"
#include "sock.h"
#include "http2.h"



struct client_s{
  enum client_state {
      NOT_CLIENT,
      CREATED,
      CONNECTED
  } state;
  sock_t * socket;
};

struct server_s{
  enum server_state {
    NOT_SERVER,
    LISTEN
  } state;
  sock_t * socket;
  struct client_s client_connect;
};

struct server_s server;
struct client_s client;


/************************************Server************************************/


int http_init_server(uint16_t port){

  client.state=NOT_CLIENT;

  sock_t server_sock;
  server.socket=&server_sock;

  if (sock_create(&server_sock)<0){
    perror("Server could not be created");
    return -1;
  }

  if (sock_listen(&server_sock, port)<0){
    perror("Partial error in server creation");
    return -1;
  }

  server.state=LISTEN;

  sock_t client_sock;

  if (sock_accept(&server_sock, &client_sock)<0){
    perror("Not client found");
    return -1;
  }

  struct client_s * cl= &server.client_connect;
  cl->state=CONNECTED;
  cl->socket=&client_sock;

  h2states_t server_state;
  if(server_init_connection(&server_state)<0){
    perror("Problems sending server data");
    return -1;
  }

  return 0;
}

int http_set_function_to_path(char * callback, char * path){
  (void) callback;
  (void) path;
  //TODO callback(argc,argv)
  return -1;
}


/************************************Client************************************/


int http_client_connect(uint16_t port, char * ip){
  struct client_s * cl= &client;
  server.state=NOT_SERVER;

  sock_t sock;
  cl->socket=&sock;

  if (sock_create(&sock)<0){
    perror("Error on client creation");
    return -1;
  }

  cl->state=CREATED;

  if (sock_connect(&sock, ip, port)<0){
    perror("Error on client connection");
    return -1;
  }

  cl->state=CONNECTED;

  h2states_t client_state;
  if(client_init_connection(&client_state)<0){
    perror("Problems sending client data");
    return -1;
  }

  return 0;
}


int http_client_disconnect(void){

  if (client.state==CREATED){
    return 0;
  }

  if (sock_destroy(client.socket)<0){
    perror("Error in client disconnection");
    return -1;
  }

  return 0;
}

int http_receive(char * headers){
  (void) headers;
  //TODO decod headers
  return -1;
}


int get_receive(char * path, char * headers){
  (void) path;
  (void) headers;
  // buscar respuesta correspondiente a path
  // codificar respuesta
  // enviar respuesta a socket
  return -1;
}



/******************************************************************************/


int http_write(uint8_t * buf, int len){
  int wr=0;

  if (client.state==NOT_CLIENT && server.state==LISTEN){
    wr= sock_write(server.socket, (char *) buf, len);
  }
  else if (server.state==NOT_SERVER){
    if (client.state != CONNECTED){
      perror("Client not connected");
      return -1;
    }
    wr= sock_write(client.socket, (char *) buf, len);
  } else {
    perror("No server or client found");
    return -1;
  }

  if (wr<=0){
    perror("Could not write");
    return wr;
  }
  return wr;
}


int http_read( uint8_t * buf, int len){
  int rd=0;

  if (client.state==NOT_CLIENT && server.state==LISTEN){
    rd = sock_read(server.socket, (char *) buf, len, 0);
  }
  else if (server.state==NOT_SERVER){
    if (client.state != CONNECTED){
      perror("Client not connected");
      return -1;
    }
    rd = sock_read(client.socket, (char *) buf, len, 0);
  } else {
    perror("No server or client found");
    return -1;
  }

  if (rd<=0){
    perror("Could not read");
    return rd;
  }
  return rd;

}
