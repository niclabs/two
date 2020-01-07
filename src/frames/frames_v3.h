//
// Created by gabriel on 06-01-20.
//

#ifndef FRAMES_V3_H
#define FRAMES_V3_H

#include <stdint.h>

#include "event.h"
#include "frames/structs.h"
#include "hpack/hpack.h"

/*Definition of max buffer size*/
#ifdef CONFIG_MAX_BUFFER_SIZE
#define FRAMES_MAX_BUFFER_SIZE (CONFIG_MAX_BUFFER_SIZE)
#else
#define FRAMES_MAX_BUFFER_SIZE 256
#endif

#define FRAME_NO_FLAG (0x0)
#define FRAME_ACK_FLAG (0x1)
#define FRAME_END_STREAM_FLAG (0x1)
#define FRAME_END_HEADERS_FLAG (0x4)
#define FRAME_PADDED_FLAG (0x8)
#define FRAME_PRIORITY_FLAG (0x20)

/*FRAME HEADER*/
typedef struct frame_header {
    uint32_t length : 24;
    enum {
        FRAME_DATA_TYPE             = (uint8_t) 0x0,
        FRAME_HEADERS_TYPE          = (uint8_t) 0x1,
        FRAME_PRIORITY_TYPE         = (uint8_t) 0x2,
        FRAME_RST_STREAM_TYPE       = (uint8_t) 0x3,
        FRAME_SETTINGS_TYPE         = (uint8_t) 0x4,
        FRAME_PUSH_PROMISE_TYPE     = (uint8_t) 0x5,
        FRAME_PING_TYPE             = (uint8_t) 0x6,
        FRAME_GOAWAY_TYPE           = (uint8_t) 0x7,
        FRAME_WINDOW_UPDATE_TYPE    = (uint8_t) 0x8,
        FRAME_CONTINUATION_TYPE     = (uint8_t) 0x9
    } type : 8;
    uint8_t flags : 8;
    uint8_t reserved : 1;
    uint32_t stream_id : 31;
} frame_header_v3_t; //72 bits-> 9 bytes


/*frame header methods*/
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array);

void frame_parse_header(frame_header_v3_t *header, uint8_t *data, unsigned int size);

// send frame methods
int send_ping_frame(event_sock_t *socket, uint8_t *opaque_data, int ack, event_write_cb cb);
int send_goaway_frame(event_sock_t *socket, uint32_t error_code, uint32_t last_open_stream_id, event_write_cb cb);
int send_settings_frame(event_sock_t *socket, int ack, uint32_t settings_values[], event_write_cb cb);
int send_headers_frame(event_sock_t *socket,
                       header_list_t* headers_list,
                       hpack_states_t* hpack_states,
                       uint32_t stream_id,
                       uint8_t end_stream,
                       event_write_cb cb);
int send_window_update_frame(event_sock_t *socket, uint8_t window_size_increment, uint32_t stream_id, event_write_cb cb);
int send_rst_stream_frame(event_sock_t *socket, uint32_t error_code, uint32_t stream_id, event_write_cb cb);
#endif //TWO_FRAMES_V3_H
