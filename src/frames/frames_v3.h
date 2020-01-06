//
// Created by gabriel on 06-01-20.
//

#ifndef FRAMES_V3_H
#define FRAMES_V3_H

#include <stdint.h>
#include "event.h"
#include "headers_frame.h"
#include "continuation_frame.h"
#include "data_frame.h"
#include "goaway_frame.h"
#include "window_update_frame.h"
#include "settings_frame.h"
#include "ping_frame.h"
#include "rst_stream_frame.h"
#include "hpack/hpack.h"


/*frame header methods*/
int frame_header_to_bytes_v3(frame_header_t *frame_header, uint8_t *byte_array);

int send_ping_frame(event_sock_t *socket, event_write_cb cb, uint8_t *opaque_data, int8_t ack);
int send_goaway_frame(event_sock_t *socket,
                      event_write_cb cb,
                      uint32_t error_code,
                      uint32_t last_open_stream_id);

int send_settings_frame(event_sock_t *socket, event_write_cb cb, uint8_t ack, uint32_t settings_values[]);

#endif //TWO_FRAMES_V3_H
