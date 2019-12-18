//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_CONTINUATION_FRAME_H
#define TWO_CONTINUATION_FRAME_H
#include "structs.h"

/*CONTINUATION FRAME*/

typedef struct {
    uint8_t *header_block_fragment; // only if length > 0. Size = frame size
}continuation_payload_t;

typedef enum __attribute__((__packed__)){
    CONTINUATION_END_HEADERS_FLAG = 0x4//bit 2
}continuation_flag_t;

/*continuation payload methods*/
void create_continuation_frame(uint8_t *headers_block, int headers_block_size, uint32_t stream_id, frame_header_t *frame_header, continuation_payload_t *continuation_payload, uint8_t *header_block_fragment);
int continuation_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_continuation_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);

#endif //TWO_CONTINUATION_FRAME_H
