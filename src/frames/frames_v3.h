//
// Created by gabriel on 06-01-20.
//

#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>
#include "headers_frame.h"
#include "continuation_frame.h"
#include "data_frame.h"
#include "goaway_frame.h"
#include "window_update_frame.h"
#include "settings_frame.h"
#include "ping_frame.h"
#include "rst_stream_frame.h"
#include "hpack/hpack.h"
#include "http2/structs.h"


/*frame header methods*/
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array);
int frame_header_from_bytes(uint8_t *byte_array, int size, frame_header_t *frame_header);

uint32_t get_header_block_fragment_size(frame_header_t *frame_header, headers_payload_t *headers_payload);
int receive_header_block(uint8_t *header_block_fragments, int header_block_fragments_pointer, header_list_t *headers, hpack_states_t *hpack_states);

/*flags methods*/
int is_flag_set(uint8_t flags, uint8_t flag);
uint8_t set_flag(uint8_t flags, uint8_t flag_to_set);

/*frame methods*/
int frame_to_bytes(frame_t *frame, uint8_t *bytes);


/*Headers compression*/
//TODO
int compress_headers(header_list_t *headers_out,  uint8_t *compressed_headers, hpack_states_t *hpack_states);

int send_ping_frame(event_sock_t *socket, uint8_t *opaque_data, int8_t ack);
int send_goaway_frame(event_sock_t *socket,
                      uint8_t flag_bits,
                      uint32_t error_code,
                      uint32_t last_open_stream_id);
#endif //TWO_FRAMES_V3_H
