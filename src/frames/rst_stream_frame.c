//
// Created by gabriel on 19-11-19.
//

#include "rst_stream_frame.h"
#include "utils.h"

#include "config.h"
#define LOG_MODULE LOG_MODULE_FRAME
#include "logging.h"


/*
 * Function: create_rst_stream_frame
 * Create a rst stream Frame
 * Input: frame_header, rst_stream_payload_t pointer, stream_id, error_code.
 */
void create_rst_stream_frame(frame_header_t *frame_header, rst_stream_payload_t *rst_stream_payload,
                             uint32_t stream_id, uint32_t error_code)
{
    frame_header->stream_id = stream_id;
    frame_header->type = RST_STREAM_TYPE;
    frame_header->length = 4;
    frame_header->flags = 0;
    frame_header->reserved = 0;
    frame_header->callback_payload_to_bytes = rst_stream_payload_to_bytes;

    rst_stream_payload->error_code = error_code;
}

/*
 * Function: rst_stream_payload_to_bytes
 * Passes a RST STREAM payload to a byte array
 * Input: frame_header, rst_stream_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int rst_stream_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    rst_stream_payload_t *rst_stream_payload = (rst_stream_payload_t *)payload;

    if (frame_header->length < 4) {
        ERROR("Length< 4, FRAME_SIZE_ERROR");
        return -1;
    }
    int pointer = 0;
    int rc = uint32_to_byte_array(rst_stream_payload->error_code, byte_array + pointer);
    if (rc < 0) {
        ERROR("error while passing uint32 to byte_array");
        return -1;
    }
    pointer += 4;
    return pointer;
}

/*
 * Function: read_rst_stream_payload
 * given a byte array, get the RST STREAM payload encoded in it
 * Input: byte_array, frame_header, rst_stream_payload
 * Output: bytes read or -1 if error
 */
int read_rst_stream_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    DEBUG("Reading RST PAYLOAD payload");
    rst_stream_payload_t *rst_stream_payload = (rst_stream_payload_t *)payload;
    if (frame_header->length != 4) {
        ERROR("Length != 4, FRAME_SIZE_ERROR");
        return -1;
    }
    if (frame_header->stream_id == 0x0) {
        ERROR("stream_id of RST STREAM FRAME is 0, PROTOCOL_ERROR");
        return -1;
    }
    uint32_t error_code = bytes_to_uint32(bytes);
    rst_stream_payload->error_code = error_code;

    return frame_header->length;
}
