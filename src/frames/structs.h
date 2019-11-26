#ifndef FRAMES_STRUCTS_H
#define FRAMES_STRUCTS_H
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

/*FRAME*/
typedef struct {
    frame_header_t *frame_header;
    void *payload;
}frame_t;

/*ERROR*/
/*Error codes*/
typedef enum {
    FRAMES_NO_ERROR                     = (int32_t) 0,
    FRAMES_PROTOCOL_ERROR               = (int32_t) - 1,
    FRAMES_INTERNAL_ERROR               = (int32_t) - 2,
    FRAMES_FRAME_SIZE_ERROR             = (int32_t) - 3,
    FRAMES_FRAME_NOT_FOUND_ERROR        = (int32_t) - 4,
    FRAMES_FRAME_NOT_IMPLEMENTED_ERROR  = (int32_t) - 5,
}frames_error_code_t;

#endif /*FRAMES_STRUCTS_H*/
