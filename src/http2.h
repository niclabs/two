#include "http2utils.h"

/*Struct for storing HTTP2 states*/
typedef struct H2States{
  uint32_t remote_settings[6];
  uint32_t local_settings[6];
  /*uint32_t local_cache[6]; Could be implemented*/
  uint8_t wait_setting_ack;
}h2states_t;

/*Default settings values*/
#define DEFAULT_HTS 4096
#define DEFAULT_EP 1
#define DEFAULT_MCS 99999
#define DEFAULT_IWS 65535
#define DEFAULT_MFS 16384
#define DEFAULT_MHLS 99999

/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1

int send_local_settings(h2states_t *st);
uint32_t read_setting_from(uint8_t place, uint8_t param, h2states_t *st);
int client_init_connection(h2states_t *st);
int server_init_connection(h2states_t *st);
int receive_frame(h2states_t *st);
