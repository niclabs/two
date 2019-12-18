//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_DATA_FRAME_H
#define TWO_DATA_FRAME_H

#include <stdint.h>
#include "structs.h"
/*DATA FRAME*/
#pragma pack(push, 1)

typedef struct {
    uint8_t pad_length;
    uint8_t *data;
    uint8_t *padding;
}data_payload_t;

typedef enum __attribute__((__packed__)){
    DATA_END_STREAM_FLAG    = 0x1,  //bit 0
    DATA_PADDED_FLAG        = 0x8   //bit 3
}data_flag_t;

#pragma pack(pop)

/*Data frame methods*/
void create_data_frame(frame_header_t *frame_header, data_payload_t *data_payload, uint8_t *data, uint8_t *data_to_send, int length, uint32_t stream_id);
int data_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_data_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);

#endif //TWO_DATA_FRAME_H
