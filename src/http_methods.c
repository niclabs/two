/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <netinet/in.h>

#include "http_methods.h"
#include "sock.h"


struct client_s{
  enum client_state {
      EMPTY_CLIENT,
      CREATED,
      CONNECTED
  } state;
  sock_t * socket;
};

struct server_s{
  enum server_state {
    EMPTY_SERVER,
    LISTEN
  } state;
  sock_t * socket;
  struct client_s client_connect;
};

struct server_s server;
struct client_s client;

/************************************Server************************************/

int http_init_server(uint16_t port){
  struct server_s * sr= &server;
  struct client_s * empty_client= &client;
  empty_client->state=EMPTY_CLIENT;

  sock_t * server_sock;

  if (sock_create(server_sock)<0){
    puts("Server could not be created");
    return -1;
  }

  if (sock_listen(server_sock, port)<0){
    puts("Partial error in server creation");
    return -1;
  }

  sr->state=CREATED;
  sr->socket=server_sock;

  sock_t * client_sock;

  if (sock_accept(sr->socket, client_sock)<0){
    puts("Not client found");
    return 0;
  }

  struct client_s * cl= &server.client_connect;
  cl->state=CONNECTED;
  cl->socket=client_sock;

  if(server_init_connection()<0){
    puts("");
  }

  return 0;
}

int http_set_function_to_path(char * callback, char * path){
  //TODO callback(argc,argv)
  return -1;
}

/************************************Client************************************/


int http_client_connect(uint16_t port, char * ip){
  struct client_s * cl= &client;
  struct server_s * sr= &server;
  sr->state=EMPTY_CLIENT;

  sock_t * sock;

  if (sock_create(sock)<0){
    puts("Client could not be created");
    return -1;
  }

  cl->socket=sock;
  cl->state=CREATED;

  if (sock_connect(sock, ip, port)<0){
    puts("Client could not be connected");
    return -1;
  }

  cl->state=CONNECTED;

  if(client_init_connection()<0){
    puts("");
  }

  return 0;
}


int http_client_disconnect(void){

  if (client.state==CREATED){
    return 0;
  }

  if (sock_destroy(client.socket)<0){
    puts("Client could not be disconnected");
    return -1;
  }

  return 0;
}

int http_receive(char * headers){
  //TODO decod headers
  return -1;
}

int get_receive(char * path, char * headers){
  // buscar respuesta correspondiente a path
  // codificar respuesta
  // enviar respuesta a socket
  return -1;
}

/******************************************************************************/


int http_write(char * buf, int len){
  if (client.state==EMPTY_CLIENT && server.state==EMPTY_CLIENT){
    puts("Could not write");
    return -1;
  }

  sock_t * sock;

  if (client.state==EMPTY_CLIENT && server.state==CREATED){
    sock = server.socket;
  }
  if (server.state==EMPTY_CLIENT){
    if (client.state != CONNECTED){
      puts("Client not connected");
      return -1;
    }
    sock = client.socket;
  }

  int wr= sock_write(sock, buf, len);
  if (wr<=0){
    puts("Could not write");
    return wr;
  }
  return wr;
}


int http_read(char * buf, int len){
  if (client.state==EMPTY_CLIENT && server.state==EMPTY_CLIENT){
    puts("Could not read");
    return -1;
  }

  sock_t * sock;

  if (client.state==EMPTY_CLIENT && server.state==CREATED){
    sock = server.socket;
  }
  if (server.state==EMPTY_CLIENT){
    if (client.state != CONNECTED){
      puts("Client not connected");
      return -1;
    }
    sock = client.socket;
  }

  int rd =  sock_read(sock, buf, len, 0);
  if (rd<=0){
    puts("Could not read");
    return rd;
  }
  return rd;

}
