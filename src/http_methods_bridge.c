/*
This API contains the HTTP methods to be used by
HTTP/2
*/

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http_methods_bridge.h"
#include "http_methods.h"
#include "logging.h"


int http_write(uint8_t * buf, int len, hstates_t * hs){
  int wr=0;

  if (hs->socket_state==1){
    wr= sock_write(hs->socket, (char *) buf, len);
  } else {
    ERROR("No client connected found");
    return -1;
  }

  if (wr<=0){
    ERROR("Error in writing");
    if (wr==0){
      hs->socket_state=0;
    }
    return wr;
  }
  return wr;
}


int http_read( uint8_t * buf, int len, hstates_t * hs){
  int rd=0;

  if (hs->socket_state==1){
    rd= sock_read(hs->socket, (char *) buf, len, 0);
  } else {
    ERROR("No client connected found");
    return -1;
  }

  if (rd<=0){
    ERROR("Error in reading");
    if (rd==0){
      hs->socket_state=0;
    }
    return rd;
  }
  return rd;

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


int http_clear_header_list(hstates_t * hs, int index){
  if (index<=10 && index>=0){
    strcpy(hs->header_list[index]->name, "");
    strcpy(hs->header_list[index]->value, "");

    if (memcmp(hs->header_list[index]->name,"",1)!=0 || memcmp(hs->header_list[index]->value,"",1)!=0){
      ERROR("Problems deleting a header or its value");
      return -1;
    }
    
    return 0;
  }

  int i;

  for (i=0, i<=hs->table_index, i++){

    strcpy(hs->header_list[i]->name, "");
    strcpy(hs->header_list[i]->value, "");

    if (memcmp(hs->header_list[i]->name,"",1)!=0 || memcmp(hs->header_list[i]->value,"",1)!=0){
      ERROR("Problems deleting a header or its value");
      return -1;
    }
  }

  hs->table_index=-1;

  return 0;
}
