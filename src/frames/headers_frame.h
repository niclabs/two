//
// Created by gabriel on 04-11-19.
//

#ifndef TWO_HEADERS_H
#define TWO_HEADERS_H
#include "structs.h"
/*HEADERS FRAME*/

#pragma pack(push, 1)
typedef struct {
    uint8_t pad_length;                 // only if padded flag is set
    uint8_t exclusive_dependency : 1;   // only if priority flag is set
    uint32_t stream_dependency : 31;    // only if priority flag is set
    uint8_t weight;                     // only if priority flag is set
    uint8_t *header_block_fragment;     // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    uint8_t *padding;                   //only if padded flag is set. Size = pad_length
}headers_payload_t;                     //48+32+32 bits -> 14 bytes
#pragma pack(pop)

typedef enum __attribute__((__packed__)){
    HEADERS_END_STREAM_FLAG     = 0x1,  //bit 0
    HEADERS_END_HEADERS_FLAG    = 0x4,  //bit 2
    HEADERS_PADDED_FLAG         = 0x8,  //bit 3
    HEADERS_PRIORITY_FLAG       = 0x20  //bit 5
}headers_flag_t;

/*header payload methods*/
void create_headers_frame(uint8_t *headers_block, int headers_block_size, uint32_t stream_id, frame_header_t *frame_header, headers_payload_t *headers_payload, uint8_t *header_block_fragment);
int headers_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array);
int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);
uint32_t get_header_block_fragment_size(frame_header_t *frame_header, headers_payload_t *headers_payload);

#endif //TWO_HEADERS_H
