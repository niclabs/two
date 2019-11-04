//
// Created by gabriel on 04-11-19.
//

#include "goaway_frame.h"
#include "logging.h"
#include "http2/utils.h"
#include "utils.h"

/*
 * Function: goaway_payload_to_bytes
 * Passes a GOAWAY payload to a byte array
 * Input: frame_header, GOAWAY_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int goaway_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    goaway_payload_t *goaway_payload = (goaway_payload_t *) payload;
    if (frame_header->length < 4) {
        ERROR("Length< 4, FRAME_SIZE_ERROR");
        return -1;
    }
    int pointer = 0;
    int rc = uint32_31_to_byte_array(goaway_payload->last_stream_id, byte_array + pointer);
    if (rc < 0) {
        ERROR("error while passing uint32_31 to byte_array");
        return -1;
    }
    pointer += 4;
    rc = uint32_to_byte_array(goaway_payload->error_code, byte_array + pointer);
    if (rc < 0) {
        ERROR("error while passing uint32 to byte_array");
        return -1;
    }
    pointer += 4;
    uint32_t additional_debug_data_size = frame_header->length - 8;
    rc = buffer_copy(byte_array + pointer, goaway_payload->additional_debug_data, additional_debug_data_size);
    if (rc < 0) {
        ERROR("error in buffer copy");
        return -1;
    }
    pointer += rc;
    return pointer;
}

/*
 * Function: create_goaway_frame
 * Create a GOAWAY Frame
 * Input: frame_header, goaway_payload, pointer to space for additional debug data, last stream id, error code, additional debug data, and its size.
 * Output: 0, or -1 if any error
 */
int create_goaway_frame(frame_header_t *frame_header, goaway_payload_t *goaway_payload, uint8_t *additional_debug_data_buffer, uint32_t last_stream_id, uint32_t error_code,  uint8_t *additional_debug_data, uint8_t additional_debug_data_size)
{
    frame_header->stream_id = 0;
    frame_header->type = GOAWAY_TYPE;
    frame_header->length = 8 + additional_debug_data_size;
    frame_header->flags = 0;
    frame_header->reserved = 0;
    frame_header->callback_to_bytes = goaway_payload_to_bytes;

    goaway_payload->last_stream_id = last_stream_id;
    goaway_payload->error_code = error_code;
    buffer_copy(additional_debug_data_buffer, additional_debug_data, additional_debug_data_size);
    goaway_payload->additional_debug_data = additional_debug_data_buffer;
    return 0;

}

/*
 * Function: read_goaway_payload
 * given a byte array, get the goaway payload encoded in it
 * Input: byte_array, frame_header, goaway_payload
 * Output: bytes read or -1 if error
 */
int read_goaway_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    goaway_payload_t *goaway_payload = (goaway_payload_t *) payload;
    if (frame_header->length < 4) {
        ERROR("Length < 4, FRAME_SIZE_ERROR");
        return -1;
    }
    int pointer = 0;
    uint32_t last_stream_id = bytes_to_uint32_31(bytes + pointer);
    pointer += 4;
    goaway_payload->last_stream_id = last_stream_id;
    uint32_t error_code = bytes_to_uint32(bytes + pointer);
    goaway_payload->error_code = error_code;
    pointer += 4;

    uint32_t additional_debug_data_size = frame_header->length - 8;
    int rc = buffer_copy(goaway_payload->additional_debug_data, bytes + pointer, additional_debug_data_size);
    if (rc < 0) {
        ERROR("error in buffer copy");
        return -1;
    }
    return frame_header->length;
}
