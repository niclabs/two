#include <string.h>
#include <stdint.h>
#include "frames.h"
#include "utils.h"

#include "config.h"

#define LOG_MODULE LOG_MODULE_FRAME
#include "logging.h"


frames_error_code_t check_frame_errors(frame_header_t *frame_header)
{
    uint8_t type = frame_header->type;
    uint32_t length = frame_header->length;
    uint32_t stream_id = frame_header->stream_id;

    switch (type) {
        case DATA_TYPE:
        case HEADERS_TYPE: {
            if (stream_id == 0x0) {
                ERROR("stream_id of HEADERS FRAME is 0, PROTOCOL_ERROR");
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case RST_STREAM_TYPE: {
            if (length != 4) {
                ERROR("RST STREAM FRAME with Length != 4, FRAME_SIZE_ERROR");
                return FRAMES_FRAME_SIZE_ERROR;
            }
            if (stream_id == 0x0) {
                ERROR("stream_id of RST STREAM FRAME is 0, PROTOCOL_ERROR");
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case SETTINGS_TYPE: {
            if (length % 6 != 0) {
                ERROR("SETTINGS FRAME length not divisible by 6, %u", length);
                return FRAMES_FRAME_SIZE_ERROR;
            }
            if (stream_id != 0x0) {
                ERROR("stream_id of SETTINGS FRAME is not 0, PROTOCOL_ERROR");
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case PING_TYPE: {
            if (length != 8) {
                ERROR("PING frame with Length != 8, FRAME_SIZE_ERROR");
                return FRAMES_FRAME_SIZE_ERROR;
            }
            if (stream_id != 0x0) {
                //Protocol ERROR
                ERROR("PING frame with stream_id %d, PROTOCOL_ERROR", frame_header->stream_id);
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case PRIORITY_TYPE: //NOT IMPLEMENTED YET
        case PUSH_PROMISE_TYPE:
            return FRAMES_FRAME_NOT_IMPLEMENTED_ERROR;
        case GOAWAY_TYPE: {
            if (length < 8) {
                ERROR("GOAWAY frame with length < 8, %u", length);
                return FRAMES_FRAME_SIZE_ERROR;
            }
            if (stream_id != 0x0) {
                //Protocol ERROR
                ERROR("GOAWAY frame with stream_id %d, PROTOCOL_ERROR", frame_header->stream_id);
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case WINDOW_UPDATE_TYPE: {
            if (length != 4) {
                ERROR("WINDOW UPDATE with length != 4, %u", length);
                return FRAMES_FRAME_SIZE_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        case CONTINUATION_TYPE: {
            if (stream_id == 0x0) {
                ERROR("stream_id of CONTINUATION FRAME is 0, PROTOCOL_ERROR");
                return FRAMES_PROTOCOL_ERROR;
            }
            return FRAMES_NO_ERROR;
        }
        default:
            WARN("Frame type %d not found", type);
            return FRAMES_FRAME_NOT_FOUND_ERROR;
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
    /*
    int errors = check_frame_errors(frame_header);

    if (errors < 0) {
        //Error in frame
        return errors;
    }*/

    //uint8_t frame_header_bytes[9];
    int size = frame_header_to_bytes(frame_header, bytes);
    //uint8_t bytes_array[frame_header->length];
    size += frame_header->callback_payload_to_bytes(frame_header, frame->payload, bytes+size);
    //int new_size = append_byte_arrays(bytes, frame_header_bytes, bytes_array, frame_header_bytes_size, size);
    return size;
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
 * Function: receive_header_block
 * receives the header block and save the headers in the headers_data_list
 * Input: header_block, header_block_size, header_data_list
 * Output: block_size or -1 if error
 */
int receive_header_block(uint8_t *header_block_fragments, int header_block_fragments_pointer, header_list_t *headers, hpack_dynamic_table_t *dynamic_table) //return size of header_list (header_count)
{
    return decode_header_block(dynamic_table, header_block_fragments, header_block_fragments_pointer, headers);
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
        ERROR("frameHeader size too small, %d", size);
        return -1;
    }
    frame_header->length = bytes_to_uint32_24(byte_array);
    frame_header->type = (frame_type_t)(byte_array[3]);
    frame_header->flags = (uint8_t)(byte_array[4]);
    frame_header->stream_id = bytes_to_uint32_31(byte_array + 5);
    frame_header->reserved = (uint8_t)((byte_array[5]) >> (uint8_t) 7);

    int errors = check_frame_errors(frame_header);

    if (errors < 0) {
        //Error in frame
        frame_header->callback_payload_from_bytes = NULL;
        return errors;
    }

    switch (frame_header->type) {
        case DATA_TYPE:
            frame_header->callback_payload_from_bytes = read_data_payload;
            break;
        case HEADERS_TYPE:
            frame_header->callback_payload_from_bytes = read_headers_payload;
            break;
        case RST_STREAM_TYPE:
            frame_header->callback_payload_from_bytes = read_rst_stream_payload;
            break;
        case SETTINGS_TYPE:
            frame_header->callback_payload_from_bytes = read_settings_payload;
            break;
        case PING_TYPE:
            frame_header->callback_payload_from_bytes = read_ping_payload;
            break;
        case GOAWAY_TYPE:
            frame_header->callback_payload_from_bytes = read_goaway_payload;
            break;
        case WINDOW_UPDATE_TYPE:
            frame_header->callback_payload_from_bytes = read_window_update_payload;
            break;
        case CONTINUATION_TYPE:
            frame_header->callback_payload_from_bytes = read_continuation_payload;
            break;
        default:
            //This is unreachable
            return 0;
    }

    return 0;
}
