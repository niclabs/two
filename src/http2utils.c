#include "http2utils.h"


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
