//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_SETTINGS_FRAME_H
#define TWO_SETTINGS_FRAME_H
#include "common.h"

/*SETTINGS FRAME*/
typedef struct {
    uint16_t identifier;
    uint32_t value;
}settings_pair_t; //48 bits -> 6 bytes

typedef struct {
    settings_pair_t *pairs;
    int count;
}settings_payload_t; //32 bits -> 4 bytes

typedef enum {
    SETTINGS_ACK_FLAG = 0x1
}setting_flag_t;

/*settings methods*/
int create_list_of_settings_pair(uint16_t *ids, uint32_t *values, int count, settings_pair_t *pair_list);
int setting_to_bytes(settings_pair_t *setting, uint8_t *byte_array);
int settings_frame_to_bytes(settings_payload_t *settings_frame, uint32_t count, uint8_t *byte_array);
int bytes_to_settings_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);

#endif //TWO_SETTINGS_FRAME_H