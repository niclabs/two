#include "http2utils.h"
#include "http_bridge.h"

/*
* Function: read_n_bytes
* Reads n bytes to buffer.
* NOTE: THIS IS A BLOCKING FUNCTION
* Input: -> buff_read: buffer where bytes are going to be written.
*        -> n: number of bytes to read.
* Output: The number of bytes written in buffer. -1 if error was found.
*/
int read_n_bytes(uint8_t *buff_read, int n,  hstates_t *hs){
  int read_bytes = 0;
  int incoming_bytes;
  while(read_bytes < n){
    incoming_bytes = http_read(hs, buff_read+read_bytes, n - read_bytes);
    /* incoming_bytes equals -1 means that there was an error*/
    if(incoming_bytes < 1){
      puts("Error in read function");
      return -1;
    }
    read_bytes = read_bytes + incoming_bytes;
  }
  return read_bytes;
}

/*
* Function: prepare_new_stream
* Prepares a new stream, setting its state as STREAM_IDLE.
* Input: -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0.
*/
int prepare_new_stream(hstates_t* st){
  uint32_t last = st->h2s.last_open_stream_id;
  if(st->is_server == 1){
    st->h2s.current_stream.stream_id = last % 2 == 0 ? last + 2 : last + 1;
    st->h2s.current_stream.state = STREAM_IDLE;
  }
  else{
    st->h2s.current_stream.stream_id = last % 2 == 0 ? last + 1 : last + 2;
    st->h2s.current_stream.state = STREAM_IDLE;
  }
  return 0;
}

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> st: pointer to hstates_t struct where settings tables are stored.
*        -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. -1 if nothing was read.
*/

uint32_t read_setting_from(hstates_t *st, uint8_t place, uint8_t param){
  if(param < 1 || param > 6){
    printf("Error: %u is not a valid setting parameter\n", param);
    return -1;
  }
  else if(place == LOCAL){
    return st->h2s.local_settings[--param];
  }
  else if(place == REMOTE){
    return st->h2s.remote_settings[--param];
  }
  else{
    puts("Error: not a valid table to read from");
    return -1;
  }
  return -1;
}
