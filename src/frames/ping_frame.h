//
// Created by gabriel on 13-11-19.
//

#ifndef TWO_PING_FRAME_H
#define TWO_PING_FRAME_H


#include "common.h"

/*GOAWAY FRAME*/
typedef struct {
    uint8_t opaque_data[8];//64 bits
} ping_payload_t;

typedef enum {
    PING_ACK_FLAG = 0x1
}ping_flag_t;

/*goaway payload methods*/
int create_ping_frame(frame_header_t *frame_header, ping_payload_t *ping_payload, uint8_t* opaque_data);
int create_ping_ack_frame(frame_header_t *frame_header, ping_payload_t *ping_payload, uint8_t* opaque_data);
int ping_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_ping_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);


#endif //TWO_PING_FRAME_H
