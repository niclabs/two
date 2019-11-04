//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_WINDOW_UPDATE_FRAME_H
#define TWO_WINDOW_UPDATE_FRAME_H

#include "common.h"


/*WINDOW_UPDATE FRAME*/
typedef struct {
    uint8_t reserved : 1;
    uint32_t window_size_increment : 31;
}window_update_payload_t;


/*Window_update frame methods*/
int create_window_update_frame(frame_header_t *frame_header, window_update_payload_t *window_update_payload, int window_size_increment, uint32_t stream_id);
int window_update_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_window_update_payload(frame_header_t *frame_header,void *payload,  uint8_t *buff_read);

#endif //TWO_WINDOW_UPDATE_FRAME_H
