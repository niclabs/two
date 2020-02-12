//
// Created by gabriel on 06-01-20.
//

#include <assert.h>
#include <string.h>

#include "frames.h"
#include "http2.h"
#include "utils.h"

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
    uint32_24_to_byte_array(frame_header->length,
                            byte_array); // length 24 bits -> bytes [0,2]
    byte_array[3] = (uint8_t)frame_header->type;  // type 8    -> bytes [3]
    byte_array[4] = (uint8_t)frame_header->flags; // flags 8   -> bytes[4]
    uint32_31_to_byte_array(frame_header->stream_id,
                            byte_array + 5); // length 31 -> bytes[5,8]
    byte_array[5] = byte_array[5] | (frame_header->reserved
                                     << (uint8_t)7); // reserved 1 -> bytes[5]

    return 9;
}

void frame_parse_header(frame_header_t *header, uint8_t *data,
                        unsigned int size)
{
    assert(size >= 9);

    // cleanup memory first
    memset(header, 0, sizeof(frame_header_t));

    // read header length
    header->length |= (data[2]);
    header->length |= (data[1] << 8);
    header->length |= (data[0] << 16);

    // read type and flags
    header->type  = data[3];
    header->flags = data[4];

    // unset the first bit of the id
    header->stream_id |= (data[5] & 0x7F) << 24;
    header->stream_id |= data[6] << 16;
    header->stream_id |= data[7] << 8;
    header->stream_id |= data[8] << 0;
}

int send_ping_frame(event_sock_t *socket, uint8_t *opaque_data, int ack,
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

int send_goaway_frame(event_sock_t *socket, uint32_t error_code,
                      uint32_t last_open_stream_id, event_write_cb cb)
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
    uint32_31_to_byte_array(last_open_stream_id, frame_bytes + frame_size);
    frame_size += 4;

    uint32_to_byte_array(error_code, frame_bytes + frame_size);
    frame_size += 4;

    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_settings_frame(event_sock_t *socket, int ack,
                        uint32_t settings_values[], event_write_cb cb)
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
        uint16_to_byte_array(identifier, frame_bytes + frame_size);
        frame_size += 2;

        uint32_t value = settings_values[i];
        uint32_to_byte_array(value, frame_bytes + frame_size);
        frame_size += 4;
    }

    // write to socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_headers_frame(event_sock_t *socket, header_list_t *headers_list,
                       hpack_dynamic_table_t *dynamic_table, uint32_t stream_id,
                       uint8_t end_stream, event_write_cb cb)
{
    // reset the reserved memory
    memset(frame_bytes, 0, FRAME_MAX_SIZE);

    // try to encode hpack into the buffer, leaving
    // space for the frame header
    int encoded_size = hpack_encode(dynamic_table, headers_list,
                                    frame_bytes + 9, FRAME_MAX_SIZE - 9);

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
                             uint8_t window_size_increment, uint32_t stream_id,
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
    uint32_31_to_byte_array(window_size_increment, frame_bytes + frame_size);
    frame_size += header.length;

    // write to the socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_rst_stream_frame(event_sock_t *socket, uint32_t error_code,
                          uint32_t stream_id, event_write_cb cb)
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
    uint32_to_byte_array(error_code, frame_bytes + frame_size);
    frame_size += header.length;

    // write to the socket
    return event_write(socket, frame_size, frame_bytes, cb);
}

int send_data_frame(event_sock_t *socket, uint8_t *data, uint32_t size,
                    uint32_t stream_id, uint8_t end_stream, event_write_cb cb)
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
