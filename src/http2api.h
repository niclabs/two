#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "frames.h"
#include "utils.h"

/*Default settings values*/
#define DEFAULT_HTS 4096
#define DEFAULT_EP 1
#define DEFAULT_MCS 99999
#define DEFAULT_IWS 65535
#define DEFAULT_MFS 16384
#define DEFAULT_MHLS 99999
/*Enumerator for settings parameters*/
enum sett_params {HEADER_TABLE_SIZE = 0x1, ENABLE_PUSH, MAX_CONCURRENT_STREAMS,
                  INITIAL_WINDOW_SIZE, MAX_FRAME_SIZE, MAX_HEADER_LIST_SIZE};


/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1
/*Definition of max buffer size*/
#define MAX_BUFFER_SIZE 265


uint8_t send_local_settings(void);
uint8_t update_settings_table(frame_t* sframe, uint8_t place);
uint8_t send_settings_ack(void);
uint32_t read_setting_from(uint8_t place, uint8_t param);
uint8_t init_connection(void);


int tcp_write(uint8_t *bytes, uint8_t length);//here just for the code to compile