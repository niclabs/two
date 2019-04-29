/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>

#include "http_methods.h"
#include "sock.h"
#include "http2api.h"


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

  int sc=sock_create(&server_sock);
  if (sc<0){
    puts("Server could not be created");
    return -1;
  }

  int sl=sock_listen(&server_sock, port);
  if (sl<0){
    puts("Partial error in server creation");
    return -1;
  }

  server.state=LISTEN;

  sock_t client_sock;

  int sa=sock_accept(&server_sock, &client_sock);
  if (sa<0){
    puts("Not client found");
    return -1;
  }

  struct client_s * cl= &server.client_connect;
  cl->state=CONNECTED;
  cl->socket=&client_sock;

  h2states_t server_state;
  if(server_init_connection(&server_state)<0){
    puts("Problems sending server data");
    return -1;
  }

  return 0;
}



/************************************Client************************************/


int http_client_connect(uint16_t port, char * ip){
  struct client_s * cl= &client;
  server.state=NOT_SERVER;

  sock_t sock;
  cl->socket=&sock;

  int sc=sock_create(&sock);
  if (sc<0){
    puts("Client could not be created");
    return -1;
  }

  cl->state=CREATED;

  int scn=sock_connect(&sock, ip, port);
  if (scn<0){
    puts("Client could not be connected");
    return -1;
  }

  cl->state=CONNECTED;

  h2states_t client_state;
  if(client_init_connection(&client_state)<0){
    puts("Problems sending client data");
    return -1;
  }

  return 0;
}


int http_client_disconnect(void){

  if (client.state==CREATED){
    return 0;
  }

  int sd=sock_destroy(client.socket);
  if (sd<0){
    puts("Client could not be disconnected");
    return -1;
  }

  return 0;
}



/******************************************************************************/


int http_write(char * buf, int len){
  if (client.state==NOT_CLIENT && server.state==NOT_SERVER){
    puts("Could not write");
    return -1;
  }

  int wr=0;

  if (client.state==NOT_CLIENT && server.state==LISTEN){
    wr= sock_write(server.socket, buf, len);
  }
  else if (server.state==NOT_SERVER){
    if (client.state != CONNECTED){
      puts("Client not connected");
      return -1;
    }
    wr= sock_write(client.socket, buf, len);
  }

  if (wr<=0){
    puts("Could not write");
    return wr;
  }
  return wr;
}


int http_read(char * buf, int len){
  if (client.state==NOT_CLIENT && server.state==NOT_SERVER){
    puts("Could not read");
    return -1;
  }

  int rd=0;

  if (client.state==NOT_CLIENT && server.state==LISTEN){
    rd = sock_read(server.socket, buf, len, 0);
  }
  else if (server.state==NOT_SERVER){
    if (client.state != CONNECTED){
      puts("Client not connected");
      return -1;
    }
    rd = sock_read(client.socket, buf, len, 0);
  }

  if (rd<=0){
    puts("Could not read");
    return rd;
  }
  return rd;

}
