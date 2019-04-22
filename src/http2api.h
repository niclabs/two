#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "frames.h"

/*Default settings values*/
#define DEFAULT_HTS 4096
#define DEFAULT_EP 1
#define DEFAULT_MCS 99999
#define DEFAULT_IWS 65535
#define DEFAULT_MFS 16384
#define DEFAULT_MHLS 99999

/*Enumerator for settings parameters*/
typedef enum SettingsParameters{
  HEADER_TABLE_SIZE = (uint16_t) 0x1,
  ENABLE_PUSH = (uint16_t) 0x2,
  MAX_CONCURRENT_STREAMS = (uint16_t) 0x3,
  INITIAL_WINDOW_SIZE = (uint16_t) 0x4,
  MAX_FRAME_SIZE = (uint16_t) 0x5,
  MAX_HEADER_LIST_SIZE = (uint16_t) 0x6
}sett_param_t;

/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1
/*Definition of max buffer size*/
#define MAX_BUFFER_SIZE 256

int send_local_settings(void);
uint32_t read_setting_from(uint8_t place, uint8_t param);
int client_init_connection(void);
int server_init_connection(void);
int receive_frame(void);
