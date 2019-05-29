#ifndef HTTP2_H
#define HTTP2_H
#include "http2utils.h"


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

typedef enum{
  STREAM_IDLE,
  STREAM_OPEN,
  STREAM_HALF_CLOSED_REMOTE,
  STREAM_HALF_CLOSED_LOCAL,
  STREAM_CLOSED
} h2_stream_state_t;

typedef struct H2_STREAM {
  uint32_t stream_id;
  h2_stream_state_t state;
} h2_stream_t;

/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    uint8_t wait_setting_ack;
    h2_stream_t current_stream;
} h2states_t;

int send_local_settings(hstates_t *hs);
uint32_t read_setting_from(uint8_t place, uint8_t param, hstates_t *st);
int client_init_connection(hstates_t *st);
int server_init_connection(hstates_t *st);
int receive_frame(hstates_t *st);
#endif /*HTTP2UTILS_H*/
