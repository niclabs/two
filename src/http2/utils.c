#include "http2/utils.h"

// Specify to which module this file belongs
#include "config.h"
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"


/*
* Function: prepare_new_stream
* Prepares a new stream, setting its state as STREAM_IDLE.
* Input: -> st: pointer to h2states_t struct where connection variables are stored
* Output: void.
*/
void prepare_new_stream(h2states_t* st){
  uint32_t last = st->last_open_stream_id;
  if(st->is_server == 1){
    st->current_stream.stream_id = last % 2 == 0 ? last + 2 : last + 1;
    st->current_stream.state = STREAM_IDLE;
  }
  else{
    st->current_stream.stream_id = last % 2 == 0 ? last + 1 : last + 2;
    st->current_stream.state = STREAM_IDLE;
  }
}

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> st: pointer to h2states_t struct where settings tables are stored.
*        -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. -1 if nothing was read.
*/

int read_setting_from(h2states_t *st, uint8_t place, uint8_t param){
  if(param < 1 || param > 6){
    DEBUG("Invalid index from which to read the settings");
    return HTTP2_RC_ERROR;
  }
  else if(place == LOCAL){
    return st->local_settings[--param];
  }
  else if(place == REMOTE){
    return st->remote_settings[--param];
  }
  else{
    DEBUG("Invalid place from where read settings");
    return HTTP2_RC_ERROR;
  }
}


int buffer_copy(uint8_t* dest, uint8_t* orig, int size){
    for(int i = 0; i < size; i++){
        dest[i] = orig[i];
    }
    return size;
}

/*
 *
 *
 */
callback_t null_callback(void){
  callback_t null_ret = {NULL, NULL};
  return null_ret;
}
