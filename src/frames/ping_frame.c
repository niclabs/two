//
// Created by gabriel on 13-11-19.
//

#include "ping_frame.h"

//
// Created by gabriel on 04-11-19.
//

#include "goaway_frame.h"
#include "logging.h"
#include "http2/utils.h"
#include "utils.h"

/*
 * Function: ping_payload_to_bytes
 * Passes a pnig payload to a byte array
 * Input: frame_header, ping_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int ping_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    ping_payload_t *ping_payload = (ping_payload_t *)payload;

    if (frame_header->stream_id != 0x0) {
        //Protocol ERROR
        ERROR("PING frame with stream_id %d, PROTOCOL_ERROR", frame_header->stream_id);
        return -1;
    }
    if (frame_header->length != 8) {
        ERROR("PING frame with Length != 8, FRAME_SIZE_ERROR");
        return -1;
    }
    int rc = buffer_copy(byte_array, ping_payload->opaque_data, frame_header->length);
    return rc;
}

/*
 * Function: create_ping_frame
 * Create a ping Frame
 * Input: frame_header, ping_payload, pointer to space for opaque_data (it will ALWAYS send 8 bytes).
 * Output: (void)
 */
void create_ping_frame(frame_header_t *frame_header, ping_payload_t *ping_payload, uint8_t *opaque_data)
{
    frame_header->stream_id = 0;
    frame_header->type = PING_TYPE;
    frame_header->length = 8;
    frame_header->flags = 0;
    frame_header->reserved = 0;
    frame_header->callback_payload_to_bytes = ping_payload_to_bytes;

    buffer_copy(ping_payload->opaque_data, opaque_data, frame_header->length);

}

/*
 * Function: create_ping_frame
 * Create a ping Frame
 * Input: frame_header, ping_payload, pointer to space for opaque_data (it will ALWAYS send 8 bytes).
 * Output: (void)
 */
void create_ping_ack_frame(frame_header_t *frame_header, ping_payload_t *ping_payload, uint8_t *opaque_data)
{
    frame_header->stream_id = 0;
    frame_header->type = PING_TYPE;
    frame_header->length = 8;
    frame_header->flags = PING_ACK_FLAG;
    frame_header->reserved = 0;
    frame_header->callback_payload_to_bytes = ping_payload_to_bytes;

    buffer_copy(ping_payload->opaque_data, opaque_data, frame_header->length);
}

/*
 * Function: read_ping_payload
 * given a byte array, get the ping payload encoded in it
 * Input: byte_array, frame_header, ping_payload
 * Output: bytes read or -1 if error
 */
int read_ping_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    ping_payload_t *ping_payload = (ping_payload_t *)payload;

    if (frame_header->length != 8) {
        ERROR("PING frame with Length != 8, FRAME_SIZE_ERROR");
        return -1;
    }
    if (frame_header->stream_id != 0x0) {
        //Protocol ERROR
        ERROR("PING frame with stream_id %d, PROTOCOL_ERROR", frame_header->stream_id);
        return -1;
    }

    int rc = buffer_copy(ping_payload->opaque_data, bytes, 8);
    if (rc < 0) {
        ERROR("error in buffer copy");
        return -1;
    }
    return frame_header->length;
}
