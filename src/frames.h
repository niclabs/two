//
// Created by gabriel on 06-01-20.
//

#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>

#include "event.h"
#include "header_list.h"
#include "hpack/hpack.h"

/*Definition of max buffer size*/
#ifdef CONFIG_MAX_BUFFER_SIZE
#define FRAMES_MAX_BUFFER_SIZE (CONFIG_MAX_BUFFER_SIZE)
#else
#define FRAMES_MAX_BUFFER_SIZE 256
#endif

#define FRAME_FLAGS_NONE        (0x0)
#define FRAME_FLAGS_ACK         (0x1)
#define FRAME_FLAGS_END_STREAM  (0x1)
#define FRAME_FLAGS_END_HEADERS (0x4)
#define FRAME_FLAGS_PADDED      (0x8)
#define FRAME_FLAGS_PRIORITY    (0x20)

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
        FRAME_PING_TYPE              = (uint8_t) 0x6,
        FRAME_GOAWAY_TYPE           = (uint8_t) 0x7,
        FRAME_WINDOW_UPDATE_TYPE    = (uint8_t) 0x8,
        FRAME_CONTINUATION_TYPE     = (uint8_t) 0x9
    } type : 8;
    uint8_t flags : 8;
    uint8_t reserved : 1;
    uint32_t stream_id : 31;
} frame_header_t; //72 bits-> 9 bytes


void frame_parse_header(frame_header_t *header, uint8_t *data, unsigned int size);

/*
 * Function: send_ping_frame
 * Queues a write of a ping frame to the socket
 * Return the actual number of bytes queued
 */
int send_ping_frame(event_sock_t *socket, uint8_t *opaque_data, int ack, event_write_cb cb);

/*
 * Function: send_goaway_frame
 * Queues a write of a goaway frame to the socket
 * Return the actual number of bytes queued
 */
int send_goaway_frame(event_sock_t *socket, uint32_t error_code, uint32_t last_open_stream_id, event_write_cb cb);


/*
 * Function: send_settings_frame
 * Queues a write of a settings frame to the socket. 
 * The parameter settings_values expects an array of size 6 with the values for the local settings in
 * the order of their identifiers
 *
 * Return the actual number of bytes queued
 */
int send_settings_frame(event_sock_t *socket, int ack, uint32_t settings_values[], event_write_cb cb);

/*
 * Function: send_headers_frame
 * Queues a write of a headers frame to the socket. 
 * buffer given as parameter.
 * Input: ->socket: event socket
 *        ->headers_list: uncompressed list of headers
 *        ->dynanic_table: local dynamic table for compression
 *        ->stream_id: stream id to write on headers frame header
 *        ->end_stream: boolean that indicates if END_STREAM_FLAG must be set
 *        ->cb: function to call on successful data send
 * Output: actual number of bytes queued*/
int send_headers_frame(event_sock_t *socket,
                       header_list_t *headers_list,
                       hpack_dynamic_table_t *dynamic_table,
                       uint32_t stream_id,
                       uint8_t end_stream,
                       event_write_cb cb);
/*
 * Function: send_window_update_frame
 * Queues a write of a window_update frame to the socket. 
 * Input: -> socket: event socket
 *        -> window_size_increment: increment to put on window_update frame
 *        -> stream_id: identifier of the stream or 0 if it is a connection window update
 *        -> cb: function to call when data is sent
 * Output: actual number of bytes queued
 */
int send_window_update_frame(event_sock_t *socket, uint8_t window_size_increment, uint32_t stream_id, event_write_cb cb);

/*
 * Function: send_rst_stream_frame
 * Queues a write of a rst_stream frame to the socket. 
 * Input: -> socket: event socket
 *        -> error_code: reason for the rst_stream
 *        -> stream_id: identifier of the stream
 *        -> cb: function to call when data is sent
 * Output: actual number of bytes queued
 */
int send_rst_stream_frame(event_sock_t *socket, uint32_t error_code, uint32_t stream_id, event_write_cb cb);


/*
 * Function: send_data_frame
 * Queues a write of a data frame to the socket. 
 * Input: -> socket: event socket
 *        -> data: data to write
 *        -> size: size of the payload
 *        -> stream_id: identifier of the stream
 *        -> end_stream: boolean that indicates if END_STREAM_FLAG must be set
 *        -> cb: function to call when data is sent
 * Output: actual number of bytes queued
 */
int send_data_frame(event_sock_t *socket, uint8_t *data, uint32_t size, uint32_t stream_id, uint8_t end_stream, event_write_cb cb);
#endif //TWO_FRAMES_V3_H
