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

int h2_send_local_settings(hstates_t *hs);
uint32_t h2_read_setting_from(uint8_t place, uint8_t param, hstates_t *st);
int h2_client_init_connection(hstates_t *st);
int h2_server_init_connection(hstates_t *st);
int h2_receive_frame(hstates_t *st);
int h2h2_send_headers(hstates_t *st);
#endif /*HTTP2_H*/
