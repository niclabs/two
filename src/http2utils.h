#ifndef HTTP2UTILS_H
#define HTTP2UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include "frames.h"
#include "headers.h"
#include "http_bridge.h"

/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1

/*Enumerator for settings parameters*/
typedef enum SettingsParameters{
  HEADER_TABLE_SIZE = (uint16_t) 0x1,
  ENABLE_PUSH = (uint16_t) 0x2,
  MAX_CONCURRENT_STREAMS = (uint16_t) 0x3,
  INITIAL_WINDOW_SIZE = (uint16_t) 0x4,
  MAX_FRAME_SIZE = (uint16_t) 0x5,
  MAX_HEADER_LIST_SIZE = (uint16_t) 0x6
}sett_param_t;

/*Definition of max buffer size*/
#define HTTP2_MAX_BUFFER_SIZE 16384

int read_n_bytes(uint8_t *buff_read, int n,  hstates_t *hs);

/*
* Function: prepare_new_stream
* Prepares a new stream, setting its state as STREAM_IDLE.
* Input: -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0.
*/
int prepare_new_stream(hstates_t* st);

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> st: pointer to hstates_t struct where settings tables are stored.
*        -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. -1 if nothing was read.
*/
uint32_t read_setting_from(hstates_t *st, uint8_t place, uint8_t param);
#endif /*HTTP2UTILS_H*/
