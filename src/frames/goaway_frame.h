//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_GOAWAY_FRAME_H
#define TWO_GOAWAY_FRAME_H

#include "common.h"

/*GOAWAY FRAME*/
typedef struct {
    uint8_t reserved : 1;
    uint32_t last_stream_id : 31;
    uint32_t error_code;
    uint8_t *additional_debug_data;
}goaway_payload_t;


/*goaway payload methods*/
void create_goaway_frame(frame_header_t *frame_header, goaway_payload_t *goaway_payload, uint8_t *additional_debug_data_buffer, uint32_t last_stream_id, uint32_t error_code,  uint8_t *additional_debug_data, uint8_t additional_debug_data_size);
int goaway_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_goaway_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);


#endif //TWO_GOAWAY_FRAME_H
