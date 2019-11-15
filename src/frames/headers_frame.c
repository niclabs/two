//
// Created by gabriel on 04-11-19.
//
#include "headers_frame.h"
#include "config.h"
#include "http2/utils.h"
#include "logging.h"

/*
 * Function: create_headers_frame
 * Creates a headers frame, with no flags set, and with the given header block fragment atached
 * Input: header_block_fragment, its, size, streamid, pointer to the frame_header, pointer to the header_payload, pointer to the header_block_fragment that will be inside the frame
 * Output: 0 if no error ocurred
 */
int create_headers_frame(uint8_t *headers_block, int headers_block_size, uint32_t stream_id, frame_header_t *frame_header, headers_payload_t *headers_payload, uint8_t *header_block_fragment)
{
    frame_type_t type = HEADERS_TYPE;
    uint8_t flags = 0x0;
    uint8_t length = headers_block_size; //no padding, no dependency. fix if this is implemented

    frame_header->length = length;
    frame_header->type = type;
    frame_header->flags = flags;
    frame_header->stream_id = stream_id;
    frame_header->reserved = 0;
    frame_header->callback_payload_to_bytes = headers_payload_to_bytes;

    buffer_copy(header_block_fragment, headers_block, headers_block_size);
    headers_payload->header_block_fragment = header_block_fragment;
    return 0;
}


/*
 * Function: headers_payload_to_bytes
 * Passes a headers payload to a byte array
 * Input: frame_header, header_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int headers_payload_to_bytes(frame_header_t *frame_header, void* payload, uint8_t *byte_array)
{
    headers_payload_t *headers_payload = (headers_payload_t *) payload;
    int pointer = 0;

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PADDED_FLAG)){ //if padded flag is set read padding length
    //    buffer_copy(byte_array+pointer, headers_payload->padding, headers_payload->pad_length);
    //    pointer+=headers_payload->pad_length;
    //}

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PRIORITY_FLAG)){ //if priority flag is set
    //    ERROR("not implemented yet");
    //    return -1;
    //}

    //header block fragment
    int header_block_fragment_size = get_header_block_fragment_size(frame_header, headers_payload);//7(int)frame_header->length-pad_length-pointer;

    buffer_copy(byte_array + pointer, headers_payload->header_block_fragment, header_block_fragment_size);
    pointer += header_block_fragment_size;

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PADDED_FLAG)) { //if padded flag is set reasd padding
    //    int rc = buffer_copy(byte_array+pointer, headers_payload->padding, headers_payload->pad_length);
    //    pointer += rc;
    //}
    return pointer;
}


/*
 * Function: read_headers_payload
 * given a byte array, get the headers payload encoded in it
 * Input: byte_array, frame_header, headers_payload, block_fragment array, padding array
 * padding and dependencies not implemented
 * Output: bytes read or -1 if error
 */
int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    DEBUG("Reading HEADERS payload");
    headers_payload_t *headers_payload = (headers_payload_t *) payload;

    uint8_t pad_length = 0;             // only if padded flag is set
    uint8_t exclusive_dependency = 0;   // only if priority flag is set
    uint32_t stream_dependency = 0;     // only if priority flag is set
    uint8_t weight = 0;                 // only if priority flag is set

    (void)pad_length;
    (void)exclusive_dependency;
    (void)stream_dependency;
    (void)weight;
    //(void)padding;
    //uint8_t header_block_fragment[64]; // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    //uint8_t padding[32]; //only if padded flag is set. Size = pad_length

    int pointer = 0;

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PADDED_FLAG)){ //if padded flag is set read padding length
    //    pad_length = read_buffer[pointer];
    //    pointer +=1;
    //    ERROR("Header with padding");
    //}

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PRIORITY_FLAG)){ //if priority flag is set
    //   exclusive_dependency = ((uint8_t)128)&read_buffer[pointer];
    //   stream_dependency = (uint32_t) bytes_to_uint32_31(read_buffer+pointer);
    //   pointer +=4;
    //   weight = read_buffer[pointer];
    //   pointer +=1;
    //   ERROR("Header with priority");
    //}

    //header block fragment
    int header_block_fragment_size = get_header_block_fragment_size(frame_header, headers_payload);//7(int)frame_header->length-pad_length-pointer;
    if (header_block_fragment_size >= HTTP2_MAX_HBF_BUFFER) {
        ERROR("Header block fragment size longer than the space given.");
        return -1;
    }
    int rc = buffer_copy(headers_payload->header_block_fragment, bytes + pointer, header_block_fragment_size);
    pointer += rc;

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PADDED_FLAG)) { //if padded flag is set reasd padding
    //    if(pad_length>=32){
    //        ERROR("Pad lenght longer than the space given.");
    //        return -1;
    //    }
    //    rc = buffer_copy(padding, read_buffer+pointer, (int)pad_length);
    //    pointer += rc;
    //}

    //headers_payload->pad_length = pad_length;//not implemented yet!
    //headers_payload->exclusive_dependency = exclusive_dependency;//not implemented yet!
    //headers_payload->stream_dependency = stream_dependency;//not implemented yet!
    //headers_payload->weight = weight;//not implemented yet!
    //headers_payload->padding = padding;//not implemented yet!
    return pointer;
}

/*
 * Function: get_header_block_fragment_size
 * Calculates the headers block framgent size of a headers_payload
 * Input: frame_header, headers_payload
 * padding and dependencies not implemented
 * Output: block_fragment_size or -1 if error
 */
uint32_t get_header_block_fragment_size(frame_header_t *frame_header, headers_payload_t *headers_payload)
{
    int priority_length = 0;
    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PRIORITY_FLAG)){
    //    priority_length = 5;
    //}
    int pad_length = 0;

    //not implemented yet!
    //if(is_flag_set(frame_header->flags, HEADERS_PADDED_FLAG)) {
    //    pad_length = headers_payload->pad_length + 1; //plus one for the pad_length byte.
    //}
    (void)headers_payload;
    return frame_header->length - pad_length - priority_length;
}

