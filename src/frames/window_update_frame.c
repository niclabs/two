//
// Created by gabriel on 04-11-19.
//

#include "window_update_frame.h"
#include "utils.h"
#define LOG_MODULE LOG_MODULE_FRAME
#include "logging.h"

/*
 * Function: window_update_payload_to_bytes
 * Passes a window_update payload to a byte array
 * Input: frame_header, window_update_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int window_update_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    (void) frame_header;
    window_update_payload_t *window_update_payload = (window_update_payload_t *)payload;

    byte_array[0] = 0;
    int rc = uint32_31_to_byte_array(window_update_payload->window_size_increment, byte_array);
    if (rc < 0) {
        ERROR("error while passing uint32_31 to byte_array");
        return -1;
    }
    return 4;//bytes
}

/*
 * Function: create_window_update_frame
 * Creates a window_update frame
 * Input: frame_header, window_update_payload, window_size_increment, stream_id
 * Output: 0 if no error
 */
int create_window_update_frame(frame_header_t *frame_header, window_update_payload_t *window_update_payload, int window_size_increment, uint32_t stream_id)
{
    frame_header->stream_id = stream_id;
    frame_header->type = WINDOW_UPDATE_TYPE;
    frame_header->length = 4;
    frame_header->reserved = 0;
    frame_header->flags = 0;
    frame_header->callback_payload_to_bytes = window_update_payload_to_bytes;

    window_update_payload->reserved = 0;
    if (window_size_increment == 0) {
        ERROR("trying to create window_update with increment 0!. PROTOCOL_ERROR");
        return -1;
    }
    window_update_payload->window_size_increment = window_size_increment;
    return 0;
}


/*
 * Function: read_window_update_payload
 * given a byte array, get the window_update payload encoded in it
 * Input: byte_array, frame_header, window_update_payload
 * Output: bytes read or -1 if error
 */
int read_window_update_payload(frame_header_t *frame_header, void *payload,  uint8_t *buff_read)
{
    DEBUG("Reading WINDOW UPDATE payload");
    window_update_payload_t *window_update_payload = (window_update_payload_t *)payload;

    uint32_t window_size_increment = bytes_to_uint32_31(buff_read);
    window_update_payload->window_size_increment = window_size_increment;
    return frame_header->length;
}
