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

int send_local_settings(hstates_t *hs);
uint32_t read_setting_from(uint8_t place, uint8_t param, hstates_t *st);
int client_init_connection(hstates_t *st);
int server_init_connection(hstates_t *st);
int receive_frame(hstates_t *st);
#endif
