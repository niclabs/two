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
#include "sock.h"
#include "http2.h"
#include "logging.h"


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
    LISTEN,
    CLIENT_CONNECT
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
    ERROR("Server could not be created");
    return -1;
  }

  if (sock_listen(&server_sock, port)<0){
    ERROR("Partial error in server creation");
    return -1;
  }

  server.state=LISTEN;

  sock_t client_sock;

  printf("Server waiting for a client\n");


  if (sock_accept(&server_sock, &client_sock)<0){
    ERROR("Not client found");
    return -1;
  }

  client.state=CONNECTED;
  client.socket=&client_sock;

  printf("Client found and connected\n");

  h2states_t server_state;
  if(server_init_connection(&server_state)<0){
    ERROR("Problems sending server data");
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


int http_server_destroy(void){

  if (server.state==NOT_SERVER){
    WARN("Server not found");
    return -1;
  }

  if (client.state==CONNECTED){
    if (sock_destroy(client.socket)<0){
      WARN("Client still connected");
    }
  }

  if (sock_destroy(server.socket)<0){
    ERROR("Error in server disconnection");
    return -1;
  }

  server.state=NOT_SERVER;

  printf("Server destroyed\n");

  return 0;
}


/************************************Client************************************/


int http_client_connect(uint16_t port, char * ip){
  struct client_s * cl= &client;
  server.state=NOT_SERVER;

  sock_t sock;
  cl->socket=&sock;

  if (sock_create(&sock)<0){
    ERROR("Error on client creation");
    return -1;
  }

  cl->state=CREATED;

  if (sock_connect(&sock, ip, port)<0){
    ERROR("Error on client connection");
    return -1;
  }

  printf("Client connected to server\n");

  cl->state=CONNECTED;

  h2states_t client_state;
  if(client_init_connection(&client_state)<0){
    ERROR("Problems sending client data");
    return -1;
  }

  return 0;
}


int http_client_disconnect(void){

  if (client.state==CREATED){
    return 0;
  }

  if (sock_destroy(client.socket)<0){
    ERROR("Error in client disconnection");
    return -1;
  }

  client.state=NOT_CLIENT;

  printf("Client disconnected\n");

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

  if (client.state == CONNECTED){
    wr= sock_write(client.socket, (char *) buf, len);
  } else {
    ERROR("No client connected found");
    return -1;
  }

  if (wr<=0){
    ERROR("Error in writing");
    if (wr==0){
      client.state=NOT_CLIENT;
      if (server.state==CLIENT_CONNECT){
        server.state=LISTEN;
      }
    }
    return wr;
  }
  return wr;
}


int http_read( uint8_t * buf, int len){
  int rd=0;

  if (client.state == CONNECTED){
    rd = sock_read(client.socket, (char *) buf, len, 0);//Timeout 0, maybe needs to be changed (?)
  } else {
    ERROR("No client connected found");
    return -1;
  }

  if (rd<=0){
    ERROR("Error in reading");
    if (rd==0){
      client.state=NOT_CLIENT;
      if (server.state==CLIENT_CONNECT){
        server.state=LISTEN;
      }
    }
    return rd;
  }
  return rd;

}
