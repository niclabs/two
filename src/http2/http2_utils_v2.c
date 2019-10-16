#include "http2_utils_v2.h"


/*
* Function: prepare_new_stream
* Prepares a new stream, setting its state as STREAM_IDLE.
* Input: -> st: pointer to h2states_t struct where connection variables are stored
* Output: 0.
*/
int prepare_new_stream(h2states_t* st){
  uint32_t last = st->last_open_stream_id;
  if(st->is_server == 1){
    st->current_stream.stream_id = last % 2 == 0 ? last + 2 : last + 1;
    st->current_stream.state = STREAM_IDLE;
  }
  else{
    st->current_stream.stream_id = last % 2 == 0 ? last + 1 : last + 2;
    st->current_stream.state = STREAM_IDLE;
  }
  return 0;
}

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> st: pointer to h2states_t struct where settings tables are stored.
*        -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. -1 if nothing was read.
*/

uint32_t read_setting_from(h2states_t *st, uint8_t place, uint8_t param){
  if(param < 1 || param > 6){
    return -1;
  }
  else if(place == LOCAL){
    return st->local_settings[--param];
  }
  else if(place == REMOTE){
    return st->remote_settings[--param];
  }
  else{
    return -1;
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

/*
* Function: change_stream_state_end_stream_flag
* Given an h2states_t struct and a boolean, change the state of the current stream
* when a END_STREAM_FLAG is sent or received.
* Input: ->st: pointer to h2states_t struct where connection variables are stored
*        ->sending: boolean like uint8_t that indicates if current flag is sent or received
* Output: 0 if no errors were found, -1 if not
*/
int change_stream_state_end_stream_flag(uint8_t sending, cbuf_t *buf_out, h2states_t *h2s){
  int rc = 0;
  if(sending){ // Change stream status if end stream flag is sending
    if(h2s->current_stream.state == STREAM_OPEN){
      h2s->current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    }
    else if(h2s->current_stream.state == STREAM_HALF_CLOSED_REMOTE){
      h2s->current_stream.state = STREAM_CLOSED;
      if(h2s->received_goaway){
        rc = send_goaway(HTTP2_NO_ERROR, buf_out, h2s);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(h2s);
      }
    }
    return rc;
  }
  else{ // Change stream status if send stream flag is received
    if(h2s->current_stream.state == STREAM_OPEN){
      h2s->current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    }
    else if(h2s->current_stream.state == STREAM_HALF_CLOSED_LOCAL){
      h2s->current_stream.state = STREAM_CLOSED;
      if(h2s->received_goaway){
        rc = send_goaway(HTTP2_NO_ERROR, buf_out, h2s);
        if(rc < 0){
          ERROR("Error in GOAWAY sending. INTERNAL ERROR");
          return rc;
        }
      }
      else{
        rc = prepare_new_stream(h2s);
      }
    }
    return rc;
  }
}
