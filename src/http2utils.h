#ifndef HTTP2UTILS_H
#define HTTP2UTILS_H
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <stdbool.h>
#include "frames.h"
#include "http_methods_bridge.h"


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
#define MAX_BUFFER_SIZE 256

int verify_setting(uint16_t id, uint32_t value);
int read_n_bytes(uint8_t *buff_read, int n,  hstates_t *hs);
//int read_headers_payload(uint8_t *buff_read, frame_header_t *hdr, headers_payload_t *hpl);
int read_headers(headers_payload_t *hpl, table_pair_t* hlist, uint8_t count, uint8_t max_count);

#endif /*HTTP2UTILS_H*/
