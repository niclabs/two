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
        if(value == 0){
          return -1;
        }
        return 0;
    case ENABLE_PUSH:
        if(value == 1 || value == 0){
          return 0;
        }
        return -1;
    case MAX_CONCURRENT_STREAMS:
        if(value == 0){
          return -1;
        }
        return 0;
    case INITIAL_WINDOW_SIZE:
        if(value > 2147483647){
          return -1;
        }
        return 0;
    case MAX_FRAME_SIZE:
        if(value < 16777215 && value > 16384){
          return 0;
        }
        return -1;

    case MAX_HEADER_LIST_SIZE:
        if(value == 0){
          return -1;
        }
        return 0;
      default:
        return -1;
  }
}

/*
* Function: read_n_bytes
* Reads n bytes to buffer.
* NOTE: THIS IS A BLOCKING FUNCTION
* Input: -> buff_read: buffer where bytes are going to be written.
*        -> n: number of bytes to read.
* Output: The number of bytes written in buffer. -1 if error was found.
*/
int read_n_bytes(uint8_t *buff_read, int n){
  int read_bytes = 0;
  int incoming_bytes;
  while(read_bytes < n){
    incoming_bytes = tcp_read(buff_read+read_bytes, n - read_bytes);
    /* incoming_bytes equals -1 means that there was an error*/
    if(incoming_bytes == -1){
      puts("Error in read function");
      return -1;
    }
    read_bytes = read_bytes + incoming_bytes;
  }
  return read_bytes;
}

/* Toy write function*/
int tcp_write(uint8_t *bytes, uint8_t length){
  if(size+length > MAX_BUFFER_SIZE){
    return -1;
  }
  memcpy(buffer+size, bytes, length);
  size += length;
  printf("Write: buffer size is %u\n", size);
  return length;
}

/* Toy read function*/
int tcp_read(uint8_t *bytes, uint8_t length){
  if(length > size){
    length = size;
  }
  /*Write to caller*/
  memcpy(bytes, buffer, length);
  size = size - length;
  /*Move the rest of the data on buffer*/
  memcpy(buffer, buffer+length, size);
  printf("Read: buffer size is %u\n", size);
  return length;
}
