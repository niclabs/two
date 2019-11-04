//
// Created by gabriel on 04-11-19.
//

#include "continuation_frame.h"
#include "http2/utils.h"


/*
 * Function: continuation_payload_to_bytes
 * Passes a continuation payload to a byte array
 * Input: frame_header, continuation_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int continuation_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    continuation_payload_t *continuation_payload = (continuation_payload_t *) payload;
    int rc = buffer_copy(byte_array, continuation_payload->header_block_fragment, frame_header->length);

    return rc;
}


/*
 * Function: create_continuation_frame
 * Creates a continuation frame, with no flags set, and with the given header block fragment atached
 * Input: header_block_fragment, its, size, streamid, pointer to the frame_header, pointer to the continuation_payload, pointer to the header_block_fragment that will be inside the frame
 * Output: 0 if no error ocurred
 */
int create_continuation_frame(uint8_t *headers_block, int headers_block_size, uint32_t stream_id, frame_header_t *frame_header, continuation_payload_t *continuation_payload, uint8_t *header_block_fragment)
{
    uint8_t type = CONTINUATION_TYPE;
    uint8_t flags = 0x0;
    uint8_t length = headers_block_size; //no padding, no dependency. fix if this is implemented

    frame_header->length = length;
    frame_header->type = type;
    frame_header->flags = flags;
    frame_header->stream_id = stream_id;
    frame_header->reserved = 0;
    frame_header->callback_to_bytes = continuation_payload_to_bytes;

    buffer_copy(header_block_fragment, headers_block, headers_block_size);
    continuation_payload->header_block_fragment = header_block_fragment;

    return 0;
}


/*
 * Function: read_continuation_payload
 * given a byte array, get the continuation payload encoded in it
 * Input: byte_array, frame_header, continuation_payload, block_fragment array
 * Output: bytes read or -1 if error
 */
int read_continuation_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    continuation_payload_t *continuation_payload = (continuation_payload_t *) payload;
    int rc = buffer_copy(continuation_payload->header_block_fragment, bytes, frame_header->length);
    return rc;
}

