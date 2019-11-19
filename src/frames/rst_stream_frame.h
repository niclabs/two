//
// Created by gabriel on 19-11-19.
//

#ifndef TWO_RST_STREAM_H
#define TWO_RST_STREAM_H

#include "common.h"

/*RST_STREAM FRAME*/
typedef struct {
    uint32_t error_code;
}rst_stream_payload_t;

/*rst stream payload methods*/
void create_rst_stream_frame(frame_header_t *frame_header, rst_stream_payload_t *rst_stream_payload, uint32_t stream_id, uint32_t error_code);
int rst_stream_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_rst_stream_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);
#endif //TWO_RST_STREAM_H
