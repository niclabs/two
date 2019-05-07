#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdlib.h>

#include "logging.h"

#include "http2api.h"
#include "http_methods.h"


int client_init_connection(h2states_t *st){
  h2states_t * a=st;
  a->wait_setting_ack=1;
  int wr=http_write("hola",4);
  if (wr<0){
    return -1;
  }
  return 0;
}
int server_init_connection(h2states_t *st){
  h2states_t * a=st;
  a->wait_setting_ack=1;
  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);
  if (rd<0){
    return -1;
  }
  INFO("Received %s", buf);
  free(buf);
  return 0;
}
