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
#include "frames_v3.h"

/*frame header methods*/
//int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array);
int frame_header_from_bytes(uint8_t *byte_array, int size, frame_header_t *frame_header);

uint32_t get_header_block_fragment_size(frame_header_t *frame_header, headers_payload_t *headers_payload);
int receive_header_block(uint8_t *header_block_fragments, int header_block_fragments_pointer, header_list_t *headers, hpack_dynamic_table_t *dynamic_table);

/*flags methods*/
int is_flag_set(uint8_t flags, uint8_t flag);
uint8_t set_flag(uint8_t flags, uint8_t flag_to_set);

/*frame methods*/
int frame_to_bytes(frame_t *frame, uint8_t *bytes);

#endif /*FRAMES_H*/
