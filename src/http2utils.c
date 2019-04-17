#include "http2utils.h"

uint8_t buffer[MAX_BUFFER_SIZE];
int size = 0;

/* Function: verify_setting
* Verifies a pair id/value setting is correct.
* Input: -> id, the type of setting parameter
*         -> value, the value to check
* Output: 0 if pair is correct, -1 if not.
*/
int verify_setting(uint16_t id, uint32_t value){
  if(id > 6 || id < 1){
    return -1;
  }
  switch(id){
    case HEADER_TABLE_SIZE:
        if(value < 0){
          return -1;
        }
        return 0;
    case ENABLE_PUSH:
        if(value == 1 || value == 0){
          return 0;
        }
        return -1;
    case MAX_CONCURRENT_STREAMS:
        if(value < 0){
          return -1;
        }
        return 0;
    case INITIAL_WINDOW_SIZE:
        if(value > 2147483647 || value < 0){
          return -1;
        }
        return 0;
    case MAX_FRAME_SIZE:
        if(value < 16777215 && value > 16384){
          return 0;
        }
        return -1;

    case MAX_HEADER_LIST_SIZE:
        if(value < 0){
          return -1;
        }
        return 0;
  }
}

int tcp_write(uint8_t *bytes, uint8_t length){
  if(size+length > MAX_BUFFER_SIZE){
    return -1;
  }
  memcpy(buffer+size, bytes, length);
  size += length;
  return length;
}

int tcp_read(uint8_t *bytes, uint8_t length){
  if(length > size){
    length = size;
  }
  /*Write to caller*/
  memcpy(bytes, buffer, length);
  size = size - length;
  /*Move the rest of the data on buffer*/
  memcpy(buffer, buffer+length, size);
  return length;
}
