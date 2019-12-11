#ifndef HTTP2_UTILS_V2_H
#define HTTP2_UTILS_V2_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "http2/http2.h"
#include "http2/structs.h"


/*
* Function: prepare_new_stream
* Prepares a new stream, setting its state as STREAM_IDLE.
* Input: -> st: pointer to hstates_t struct where connection variables are stored
* Output: void.
*/
void prepare_new_stream(h2states_t* st);

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> st: pointer to hstates_t struct where settings tables are stored.
*        -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. -1 if nothing was read.
*/
uint32_t read_setting_from(h2states_t *st, uint8_t place, uint8_t param);

int change_stream_state_end_stream_flag(uint8_t sending, h2states_t *h2s);
#endif
