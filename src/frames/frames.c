#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "frames.h"
#include "utils.h"
#include "http2/utils.h"
#include "logging.h"
#include "hpack.h"
#include "config.h"

/*
 * Function: frame_header_to_bytes
 * Pass a frame header to an array of bytes
 * Input: pointer to frameheader, array of bytes
 * Output: 0 if bytes were read correctly, (-1 if any error reading)
 */
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array)
{
    uint8_t length_bytes[3];

    uint32_24_to_byte_array(frame_header->length, length_bytes);
    uint8_t stream_id_bytes[4];
    uint32_31_to_byte_array(frame_header->stream_id, stream_id_bytes);
    for (int i = 0; i < 3; i++) {
        byte_array[0 + i] = length_bytes[i];                        //length 24
    }
    byte_array[3] = (uint8_t)frame_header->type;                    //type 8
    byte_array[4] = (uint8_t)frame_header->flags;                   //flags 8
    for (int i = 0; i < 4; i++) {
        byte_array[5 + i] = stream_id_bytes[i];                     //stream_id 31
    }
    byte_array[5] = byte_array[5] | (frame_header->reserved << 7);  // reserved 1
    return 9;
}


int check_frame_errors(uint8_t type, uint32_t length){
    switch (type) {
        case DATA_TYPE:
        case HEADERS_TYPE:
        case CONTINUATION_TYPE:
            return 0;
        case PRIORITY_TYPE: //NOT IMPLEMENTED YET
        case RST_STREAM_TYPE:
        case PUSH_PROMISE_TYPE:
        case PING_TYPE:
            return -1;
        case SETTINGS_TYPE: {
            if (length % 6 != 0) {
                printf("Error: length not divisible by 6, %d", length);
                return -1;
            } else {
                return 0;
            }
        }
        case GOAWAY_TYPE: {
            if (length < 8) {
                printf("Error: length < 8, %d", length);
                return -1;
            } else {
                return 0;
            }
        }
        case WINDOW_UPDATE_TYPE: {
            if (length != 4) {
                printf("Error: length != 4, %d", length);
                return -1;
            } else {
                return 0;
            }
        }
        default:
            ERROR("Frame type %d not found", type);
            return -1;
    }
}

/*
 * Function: frame_to_bytes
 * pass a complete Frame(of any type) to bytes
 * Input:  Frame pointer, pointer to bytes
 * Output: size of written bytes, -1 if any error
 */
int frame_to_bytes(frame_t *frame, uint8_t *bytes)
{
    frame_header_t *frame_header = frame->frame_header;
    uint32_t length = frame_header->length;
    uint8_t type = frame_header->type;
    int errors = check_frame_errors(type, length);
    if(errors < 0){
        //Error in frame
        return errors;
    }

    uint8_t frame_header_bytes[9];
    int frame_header_bytes_size = frame_header_to_bytes(frame_header, frame_header_bytes);
    uint8_t bytes_array[length];
    int size = frame_header->callback_payload_to_bytes(frame_header, frame->payload, bytes_array);
    int new_size = append_byte_arrays(bytes, frame_header_bytes, bytes_array, frame_header_bytes_size, size);
    return new_size;
}


/*
 * Function: is_flag_set
 * Tells if a flag is set in a flag byte.
 * Input:  flag byte to test, queried flag
 * Output: 1 if queried flag was set, 0 if not
 */
int is_flag_set(uint8_t flags, uint8_t queried_flag)
{
    if ((queried_flag & flags) > 0) {
        return 1;
    }
    return 0;
}

/*
 * Function: set_flag
 * Sets a flag in a flag byte.
 * Input: flag byte, flag to set
 * Output: flag byte with flag setted
 */
uint8_t set_flag(uint8_t flags, uint8_t flag_to_set)
{
    uint8_t new_flag = flags | flag_to_set;

    return new_flag;
}

/*
 * Function: compress_headers
 * given a set of headers, it comprisses them and save them in a bytes array
 * Input: table of headers, size of the table, array to save the bytes and dynamic_table
 * Output: compressed headers size or -1 if error
 */
int compress_headers(headers_t *headers_out, uint8_t *compressed_headers, hpack_states_t *hpack_states)
{
    //TODO implement dynamic table size update
    int pointer = 0;

    for (uint8_t i = 0; i < headers_count(headers_out); i++) {
        int rc = encode(hpack_states, headers_get_name_from_index(headers_out, i), headers_get_value_from_index(headers_out, i), compressed_headers + pointer);
        pointer += rc;
    }
    return pointer;
}

/*
 * Function: receive_header_block
 * receives the header block and save the headers in the headers_data_list
 * Input: header_block, header_block_size, header_data_list
 * Output: block_size or -1 if error
 */
int receive_header_block(uint8_t *header_block_fragments, int header_block_fragments_pointer, headers_t *headers, hpack_states_t *hpack_states) //return size of header_list (header_count)
{
    int rc = decode_header_block(hpack_states, header_block_fragments, header_block_fragments_pointer, headers);

    return rc;
}


/*
 * Function: create_settings_ack_frame
 * Create a Frame of type sewttings with flag ack
 * Input:  pointer to frame, pointer to frameheader
 * Output: 0 if setting frame was created
 */
int create_settings_ack_frame(frame_t *frame, frame_header_t *frame_header)
{
    frame_header->length = 0;
    frame_header->type = SETTINGS_TYPE;//settings;
    frame_header->flags = set_flag(0x0, SETTINGS_ACK_FLAG);
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    frame->frame_header = frame_header;
    frame->payload = NULL;
    return 0;
}

/*
 * Function: bytes_to_frame_header
 * Transforms bytes to a FrameHeader
 * Input:  array of bytes, size ob bytes to read,pointer to frameheader
 * Output: 0 if bytes were written correctly, -1 if byte size is <9
 */
int frame_header_from_bytes(uint8_t *byte_array, int size, frame_header_t *frame_header)
{
    if (size < 9) {
        ERROR("frameHeader size too small, %d\n", size);
        return -1;
    }
    frame_header->length = bytes_to_uint32_24(byte_array);
    frame_header->type = (uint8_t)(byte_array[3]);
    frame_header->flags = (uint8_t)(byte_array[4]);
    frame_header->stream_id = bytes_to_uint32_31(byte_array + 5);
    frame_header->reserved = (uint8_t)((byte_array[5]) >> 7);

    switch (frame_header->type)
    {
    case WINDOW_UPDATE_TYPE:
        frame_header->callback_payload_from_bytes = read_window_update_payload;
        break;
    case DATA_TYPE:
        frame_header->callback_payload_from_bytes = read_data_payload;
        break;
    case GOAWAY_TYPE:
        frame_header->callback_payload_from_bytes = read_goaway_payload;
        break;
    case SETTINGS_TYPE:
        frame_header->callback_payload_from_bytes = read_settings_payload;
        break;
    case CONTINUATION_TYPE:
        frame_header->callback_payload_from_bytes = read_continuation_payload;
        break;
    case HEADERS_TYPE:
        frame_header->callback_payload_from_bytes = read_headers_payload;
        break;
    default:
        ERROR("Frame type %d not found", frame_header->type);
        return -1;
    }

    return 0;
}
