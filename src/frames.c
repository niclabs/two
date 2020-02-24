//
// Created by gabriel on 06-01-20.
//

#include <assert.h>
#include <string.h>

#include "buffer.h"
#include "frames.h"
#include "http2.h"

#define LOG_MODULE LOG_MODULE_FRAME
#include "logging.h"

#ifndef CONFIG_FRAME_MAX_SIZE
#define FRAME_MAX_SIZE (HTTP2_SOCK_WRITE_SIZE)
#elif (CONFIG_FRAME_MAX_SIZE) > (HTTP2_SOCK_WRITE_SIZE)
#error "Max frame size cannot be larger than http2 write buffer size"
#else
#define FRAME_MAX_SIZE (CONFIG_FRAME_MAX_SIZE)
#endif

// reserve some static memory
// to create frames
static uint8_t frame_bytes[FRAME_MAX_SIZE];

/*
 * Function: frame_header_to_bytes
 * Convert a frame header into an array of bytes
 * Input: pointer to frameheader, array of bytes
 * Output: 0 if bytes were read correctly, (-1 if any error reading)
 */
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array)
{
    buffer_put_u24(byte_array, frame_header->length);
    buffer_put_u8(byte_array + 3, frame_header->type);
    buffer_put_u8(byte_array + 4, frame_header->flags);
    buffer_put_u31(byte_array + 5, frame_header->stream_id);

    // set reserved bit to 1
    byte_array[5] |= 0x80;

    return 9;
}

void frame_parse_header(frame_header_t *header,
                        uint8_t *data,
                        unsigned int size)
{
    (void)size;
    assert(size >= 9);

    // cleanup memory first
    memset(header, 0, sizeof(frame_header_t));

    header->length    = buffer_get_u24(data);
    header->type      = buffer_get_u8(data + 3);
    header->flags     = buffer_get_u8(data + 4);
    header->stream_id = buffer_get_u31(data + 5);
}

int send_ping_frame(event_sock_t *socket,
                    uint8_t *opaque_data,
                    int ack,
                    event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // Create the frame header
    frame_header_t header;
    header.length    = 8;
    header.type      = FRAME_PING_TYPE;
    header.flags     = (uint8_t)(ack ? FRAME_FLAGS_ACK : 0);
    header.reserved  = 0;
    header.stream_id = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    /*We put the payload on the reserved buffer */
    memcpy(frame_bytes + frame_size, opaque_data, header.length);
    frame_size += header.length;

    // We write the ping to the network
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_goaway_frame(event_sock_t *socket,
                      uint32_t error_code,
                      uint32_t last_open_stream_id,
                      event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // Create the frame header
    frame_header_t header;
    header.length    = 8;
    header.type      = FRAME_GOAWAY_TYPE;
    header.flags     = 0;
    header.stream_id = 0;
    header.reserved  = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    /*We put the payload on the buffer*/
    buffer_put_u31(frame_bytes + frame_size, last_open_stream_id);
    frame_size += 4;

    buffer_put_u32(frame_bytes + frame_size, error_code);
    frame_size += 4;

    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_settings_frame(event_sock_t *socket,
                        int ack,
                        uint32_t settings_values[],
                        event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    uint8_t count  = 6;
    uint16_t ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };

    // Create the frame header
    frame_header_t header;
    header.length    = ack ? 0 : (6 * count);
    header.type      = FRAME_SETTINGS_TYPE;
    header.flags     = (uint8_t)(ack ? FRAME_FLAGS_ACK : 0);
    header.reserved  = 0x0;
    header.stream_id = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    // write payload
    for (int i = 0; i < count && !ack; i++) {
        uint16_t identifier = ids[i];
        buffer_put_u16(frame_bytes + frame_size, identifier);
        frame_size += 2;

        uint32_t value = settings_values[i];
        buffer_put_u32(frame_bytes + frame_size, value);
        frame_size += 4;
    }

    // write to socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_headers_frame(event_sock_t *socket,
                       header_list_t *headers_list,
                       hpack_dynamic_table_t *dynamic_table,
                       uint32_t stream_id,
                       uint8_t end_stream,
                       event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // try to encode hpack into the buffer, leaving
    // space for the frame header
    int encoded_size = hpack_encode(
      dynamic_table, headers_list, frame_bytes + 9, FRAME_MAX_SIZE - 9);

    if (encoded_size < 0) {
        return encoded_size;
    }

    // Create the frame header
    frame_header_t header;
    header.length = encoded_size;
    header.type   = FRAME_HEADERS_TYPE;
    header.flags  = FRAME_FLAGS_END_HEADERS; // we never send continuation
    header.flags |= (uint8_t)(end_stream ? FRAME_FLAGS_END_STREAM : 0x0);
    header.stream_id = stream_id;
    header.reserved  = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);
    frame_size += encoded_size;

    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_window_update_frame(event_sock_t *socket,
                             uint32_t window_size_increment,
                             uint32_t stream_id,
                             event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // Create the frame header
    frame_header_t header;
    header.stream_id = stream_id;
    header.type      = FRAME_WINDOW_UPDATE_TYPE;
    header.length    = 4;
    header.reserved  = 0;
    header.flags     = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    /*Then we put the payload*/
    buffer_put_u31(frame_bytes + frame_size, window_size_increment);
    frame_size += header.length;

    // write to the socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_rst_stream_frame(event_sock_t *socket,
                          uint32_t error_code,
                          uint32_t stream_id,
                          event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // Create the frame header
    frame_header_t header;
    header.stream_id = stream_id;
    header.type      = FRAME_RST_STREAM_TYPE;
    header.length    = 4;
    header.flags     = 0;
    header.reserved  = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    /*Then we put the payload*/
    buffer_put_u32(frame_bytes + frame_size, error_code);
    frame_size += header.length;

    // write to the socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_data_frame(event_sock_t *socket,
                    uint8_t *data,
                    uint32_t size,
                    uint32_t stream_id,
                    uint8_t end_stream,
                    event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // Create the frame header
    // TODO: check if frame fits in frame_size before setting
    // end stream flag?
    frame_header_t header;
    header.length    = size;
    header.type      = FRAME_DATA_TYPE;
    header.flags     = (uint8_t)(end_stream ? FRAME_FLAGS_END_STREAM : 0x0);
    header.stream_id = stream_id;
    header.reserved  = 0;

    // copy header data into the beginning of the frame
    int frame_size = frame_header_to_bytes(&header, frame_bytes);

    // copy data into the frame
    memcpy(frame_bytes + frame_size, data, size);
    frame_size += size;

    // write to the socket
    return event_write(socket, frame_size, frame_bytes, cb);
}
