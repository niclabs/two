#ifndef FRAMES_COMMON_H
#define FRAMES_COMMON_H
#include <stdint.h>

/*note the difference between
   frameheader (data that identifies a frame of any type) and
   headersframe(type of frame that contains http headers)*/

/*FRAME TYPES*/
typedef enum {
    DATA_TYPE           = (uint8_t) 0x0,
    HEADERS_TYPE        = (uint8_t) 0x1,
    PRIORITY_TYPE       = (uint8_t) 0x2,
    RST_STREAM_TYPE     = (uint8_t) 0x3,
    SETTINGS_TYPE       = (uint8_t) 0x4,
    PUSH_PROMISE_TYPE   = (uint8_t) 0x5,
    PING_TYPE           = (uint8_t) 0x6,
    GOAWAY_TYPE         = (uint8_t) 0x7,
    WINDOW_UPDATE_TYPE  = (uint8_t) 0x8,
    CONTINUATION_TYPE   = (uint8_t) 0x9
}frame_type_t;

/*FRAME HEADER*/
typedef struct FRAME_HEADER {
    uint32_t length : 24;
    frame_type_t type;
    uint8_t flags;
    uint8_t reserved : 1;
    uint32_t stream_id : 31;
    int (*callback_payload_from_bytes)(struct FRAME_HEADER *frame_header, void *payload, uint8_t *bytes);
    int (*callback_payload_to_bytes)(struct FRAME_HEADER *frame_header, void *payload, uint8_t *byte_array);
} frame_header_t; //72 bits-> 9 bytes

#endif /*FRAMES_COMMON_H*/
