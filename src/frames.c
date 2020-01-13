//
// Created by gabriel on 06-01-20.
//

#include <string.h>
#include <assert.h>

#include "frames.h"
#include "utils.h"

#define LOG_MODULE LOG_MODULE_FRAME
#include "logging.h"

/*
 * Function: frame_header_to_bytes
 * Convert a frame header into an array of bytes
 * Input: pointer to frameheader, array of bytes
 * Output: 0 if bytes were read correctly, (-1 if any error reading)
 */
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array)
{
    uint32_24_to_byte_array(frame_header->length, byte_array);                  //length 24 bits -> bytes [0,2]
    byte_array[3] = (uint8_t)frame_header->type;                                //type 8    -> bytes [3]
    byte_array[4] = (uint8_t)frame_header->flags;                               //flags 8   -> bytes[4]
    uint32_31_to_byte_array(frame_header->stream_id, byte_array + 5);           //length 31 -> bytes[5,8]
    byte_array[5] = byte_array[5] | (frame_header->reserved << (uint8_t)7);     // reserved 1 -> bytes[5]

    return 9;
}

void frame_parse_header(frame_header_t *header, uint8_t *data, unsigned int size)
{
    assert(size >= 9);

    // cleanup memory first
    memset(header, 0, sizeof(frame_header_t));

    // read header length
    header->length |= (data[2]);
    header->length |= (data[1] << 8);
    header->length |= (data[0] << 16);

    // read type and flags
    header->type = data[3];
    header->flags = data[4];

    // unset the first bit of the id
    header->stream_id |= (data[5] & 0x7F) << 24;
    header->stream_id |= data[6] << 16;
    header->stream_id |= data[7] << 8;
    header->stream_id |= data[8] << 0;
}

int send_ping_frame(event_sock_t *socket, uint8_t *opaque_data, int ack, event_write_cb cb)
{
    /*First we create the header*/
    frame_header_t header;

    //ping_payload_t ping_payload;
    header.length = 8;
    header.type = FRAME_PING_TYPE;
    header.flags = (uint8_t) (ack ? FRAME_FLAGS_ACK : 0);
    header.reserved = 0;
    header.stream_id = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + 8];     /*ping  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + 8);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    /*We put the payload on the buffer*/
    memcpy(response_bytes + size_bytes, opaque_data, header.length);
    size_bytes += header.length;

    // We write the ping to NET
    return event_write(socket, size_bytes, response_bytes, cb);
}

int send_goaway_frame(event_sock_t *socket, uint32_t error_code, uint32_t last_open_stream_id, event_write_cb cb)
{
    frame_header_t header;

    header.length = 8;
    header.type = FRAME_GOAWAY_TYPE;
    header.flags = 0;
    header.stream_id = 0;
    header.reserved = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*ping  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);
    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    /*We put the payload on the buffer*/
    uint32_31_to_byte_array(last_open_stream_id, response_bytes + size_bytes);
    size_bytes += 4;

    uint32_to_byte_array(error_code, response_bytes + size_bytes);
    size_bytes += 4;

    return event_write(socket, size_bytes, response_bytes, cb);
}

int send_settings_frame(event_sock_t *socket, int ack, uint32_t settings_values[], event_write_cb cb)
{

    uint8_t count = 6;
    uint16_t ids[] = { 0x1, 0x2, 0x3, 0x4, 0x5, 0x6 };

    frame_header_t header;

    /*rc must be 0*/
    header.length = ack ? 0 : (6 * count);
    header.type = FRAME_SETTINGS_TYPE;    //settings;
    header.flags = (uint8_t) (ack ? FRAME_FLAGS_ACK : 0);
    header.reserved = 0x0;
    header.stream_id = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*settings  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    for (int i = 0; i < count && !ack; i++) {
        uint16_t identifier = ids[i];
        uint16_to_byte_array(identifier, response_bytes + size_bytes);
        size_bytes += 2;

        uint32_t value = settings_values[i];
        uint32_to_byte_array(value, response_bytes + size_bytes);
        size_bytes += 4;
    }

    return event_write(socket, size_bytes, response_bytes, cb);
}


int send_headers_frame(event_sock_t *socket,
                       header_list_t *headers_list,
                       hpack_dynamic_table_t *dynamic_table,
                       uint32_t stream_id,
                       uint8_t end_stream,
                       event_write_cb cb)
{
    uint8_t encoded_bytes[FRAMES_MAX_BUFFER_SIZE];
    int size = hpack_encode(dynamic_table, headers_list, encoded_bytes);

    if (size < 0) { //Error
        return size;
    }
    frame_header_t header;

    // We create the headers frame
    header.length = size;
    header.type = FRAME_HEADERS_TYPE;
    header.flags = FRAME_FLAGS_END_HEADERS; // we never send continuation
    header.flags |= (uint8_t) (end_stream ? FRAME_FLAGS_END_STREAM : 0x0);
    header.stream_id = stream_id;
    header.reserved = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*settings  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    /*Then we put the payload*/
    memcpy(response_bytes + size_bytes, response_bytes, header.length);
    size_bytes += header.length;

    return event_write(socket, size_bytes, response_bytes, cb);
}

int send_window_update_frame(event_sock_t *socket, uint8_t window_size_increment, uint32_t stream_id, event_write_cb cb)
{
    frame_header_t header;

    header.stream_id = stream_id;
    header.type = FRAME_WINDOW_UPDATE_TYPE;
    header.length = 4;
    header.reserved = 0;
    header.flags = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*settings  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    /*Then we put the payload*/
    uint32_31_to_byte_array(window_size_increment, response_bytes + size_bytes);
    size_bytes += header.length;

    return event_write(socket, size_bytes, response_bytes, cb);
}

int send_rst_stream_frame(event_sock_t *socket, uint32_t error_code, uint32_t stream_id, event_write_cb cb)
{
    frame_header_t header;

    header.stream_id = stream_id;
    header.type = FRAME_RST_STREAM_TYPE;
    header.length = 4;
    header.flags = 0;
    header.reserved = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*settings  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);

    /*Then we put the payload*/
    uint32_to_byte_array(error_code, response_bytes + size_bytes);
    size_bytes += header.length;

    return event_write(socket, size_bytes, response_bytes, cb);
}

int send_data_frame(event_sock_t *socket, uint8_t *data, uint32_t size, uint32_t stream_id, uint8_t end_stream, event_write_cb cb)
{
    frame_header_t header;

    header.length = size;
    header.type = FRAME_DATA_TYPE;
    header.flags = (uint8_t) (end_stream ? FRAME_FLAGS_END_STREAM : 0x0);
    header.stream_id = stream_id;
    header.reserved = 0;

    /*Then we put it in a buffer*/
    uint8_t response_bytes[9 + header.length];     /*settings  frame has a header and a payload of 8 bytes*/
    memset(response_bytes, 0, 9 + header.length);

    int size_bytes = frame_header_to_bytes(&header, response_bytes);
    memcpy(response_bytes + size_bytes, data, header.length);
    size_bytes += header.length;

    return event_write(socket, size_bytes, response_bytes, cb);
}
