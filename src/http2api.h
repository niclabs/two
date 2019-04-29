#include "logging.h"
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <stdint.h>


/*Struct for storing HTTP2 states*/
typedef struct H2States{
  uint32_t remote_settings[6];
  uint32_t local_settings[6];
  /*uint32_t local_cache[6]; Could be implemented*/
  uint8_t wait_setting_ack;
}h2states_t;



int client_init_connection(h2states_t *st);
int server_init_connection(h2states_t *st);
