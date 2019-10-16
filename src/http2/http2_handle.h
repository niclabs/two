#include "http2_structs.h"


#ifndef HTTP2_HANDLE_H
#define HTTP2_HANDLE_H
int handle_data_payload(frame_header_t *frame_header, data_payload_t *data_payload, cbuf_t *buf_out, h2states_t* h2s);

/*
* Function: handle_headers_payload
* Does all the operations related to an incoming HEADERS FRAME.
* Input: -> header: pointer to the headers frame header (frame_header_t)
*        -> hpl: pointer to the headers payload (headers_payload_t)
*        -> st: pointer to h2states_t struct where connection variables are stored
* Output: 0 if no error was found, -1 if not.
*/
int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, cbuf_t *buf_out, h2states_t *h2s);

/*
* Function: handle_settings_payload
* Reads a settings payload from buffer and works with it.
* Input: -> buff_read: buffer where payload's data is written
        -> header: pointer to a frameheader_t structure already built with frame info
        -> spl: pointer to settings_payload_t struct where data is gonna be written
        -> pairs: pointer to settings_pair_t array where data is gonna be written
        -> st: pointer to h2states_t struct where connection variables are stored
* Output: 0 if operations are done successfully, -1 if not.
*/
int handle_settings_payload(settings_payload_t *spl, cbuf_t *buf_out, h2states_t *h2s);

/*
* Function: handle_goaway_payload
* Handles go away payload.
* Input: ->goaway_pl: goaway_payload_t pointer to goaway frame payload
*        ->st: pointer h2states_t struct where connection variables are stored
* IMPORTANT: this implementation doesn't check the correctness of the last stream
* id sent by the endpoint. That is, if a previous GOAWAY was received with an n
* last_stream_id, it assumes that the next value received is going to be the same.
* Output: 0 if no error were found during the handling, 1 if not
*/
int handle_goaway_payload(goaway_payload_t *goaway_pl, cbuf_t *buf_out, h2states_t *h2s);

/*
* Function: handle_continuation_payload
* Does all the operations related to an incoming CONTINUATION FRAME.
* Input: -> header: pointer to the continuation frame header (frame_header_t)
*        -> hpl: pointer to the continuation payload (continuation_payload_t)
*        -> st: pointer to h2states_t struct where connection variables are stored
* Output: 0 if no error was found, -1 if not.
*/
int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, cbuf_t *buf_out, h2states_t *h2s);
#endif
