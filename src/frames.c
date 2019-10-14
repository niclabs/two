#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "frames.h"
#include "utils.h"
#include "http2/http2_utils_v2.h"
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


/*
 * Function: setting_to_bytes
 * Pass a settingPair to bytes
 * Input:  settingPair, pointer to bytes
 * Output: size of written bytes
 */
int setting_to_bytes(settings_pair_t *setting, uint8_t *bytes)
{
    uint8_t identifier_bytes[2];

    uint16_to_byte_array(setting->identifier, identifier_bytes);
    uint8_t value_bytes[4];
    uint32_to_byte_array(setting->value, value_bytes);
    for (int i = 0; i < 2; i++) {
        bytes[i] = identifier_bytes[i];
    }
    for (int i = 0; i < 4; i++) {
        bytes[2 + i] = value_bytes[i];
    }
    return 6;
}

/*
 * Function: settings_frame_to_bytes
 * pass a settings payload to bytes
 * Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
 * Output: size of written bytes
 */
int settings_frame_to_bytes(settings_payload_t *settings_payload, uint32_t count, uint8_t *bytes)
{
    for (uint32_t i = 0; i < count; i++) {
        //printf("%d\n",i);
        uint8_t setting_bytes[6];
        int size = setting_to_bytes(settings_payload->pairs + i, setting_bytes);
        for (int j = 0; j < size; j++) {
            bytes[i * 6 + j] = setting_bytes[j];
        }
    }
    return 6 * count;
}

/*
 * Function: bytes_to_settings_payload
 * pass a settings payload to bytes
 * Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
 * Output: size of written bytes
 */
int bytes_to_settings_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    settings_payload_t *settings_payload = (settings_payload_t *) payload;
    if (frame_header->length % 6 != 0) {
        printf("ERROR: settings payload wrong size\n");
        return -1;
    }
    int count = frame_header->length / 6;

    for (int i = 0; i < count; i++) {
        settings_payload->pairs[i].identifier = bytes_to_uint16(bytes + (i * 6));
        settings_payload->pairs[i].value = bytes_to_uint32(bytes + (i * 6) + 2);
    }
    settings_payload->count = count;
    return (6 * count);
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
                ERROR("Length not divisible by 6, %d", length);
                return -1;
            } else {
                return 0;
            }
        }
        case GOAWAY_TYPE: {
            if (length < 8) {
                ERROR("length < 8, %d", length);
                return -1;
            } else {
                return 0;
            }
        }
        case WINDOW_UPDATE_TYPE: {
            if (length != 4) {
                ERROR("length != 4, %d", length);
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
    int size = frame_header->payload_callback(frame_header, frame->payload, bytes_array);
    int new_size = append_byte_arrays(bytes, frame_header_bytes, bytes_array, frame_header_bytes_size, size);
    return new_size;

}


/*
 * Function: create_list_of_settings_pair
 * Create a List Of SettingsPairs
 * Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to settings Pairs
 * Output: size of read setting pairs
 */
int create_list_of_settings_pair(uint16_t *ids, uint32_t *values, int count, settings_pair_t *settings_pairs)
{
    for (int i = 0; i < count; i++) {
        settings_pairs[i].identifier = ids[i];
        settings_pairs[i].value = values[i];
    }
    return count;
}

/*
 * Function: create_settings_frame
 * Create a Frame of type sewttings with its payload
 * Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to frame, pointer to frameheader, pointer to settings payload, pointer to settings Pairs.
 * Output: 0 if setting frame was created
 */
int create_settings_frame(uint16_t *ids, uint32_t *values, int count, frame_t *frame, frame_header_t *frame_header,
                          settings_payload_t *settings_payload, settings_pair_t *pairs)
{
    frame_header->length = count * 6;
    frame_header->type = SETTINGS_TYPE;//settings;
    frame_header->flags = 0x0;
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    count = create_list_of_settings_pair(ids, values, count, pairs);
    settings_payload->count = count;
    settings_payload->pairs = pairs;
    frame->payload = (void *)settings_payload;
    frame->frame_header = frame_header;
    return 0;
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
    frame_header->payload_callback = headers_payload_to_bytes;

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
    frame_header->payload_callback = continuation_payload_to_bytes;

    buffer_copy(header_block_fragment, headers_block, headers_block_size);
    continuation_payload->header_block_fragment = header_block_fragment;

    return 0;
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
   int compress_headers_with_strategy(char* headers, int headers_size, uint8_t* compressed_headers, uint8_t bool_table_compression, uint8_t bool_huffman_compression){
    //TODO implement
    (void)headers;
    (void)headers_size;
    (void)compressed_headers;
    (void)bool_table_compression;
    (void)bool_huffman_compression;
    return -1;
    if(bool_table_compression>0){
        return compress_static(headers, headers_size, compressed_headers);
    }
    if(bool_huffman_compression>0){
        return compress_huffman(headers, headers_size, compressed_headers);
    }
    //without compression
    for(int i =0; i<headers_size; i++){
        compressed_headers[i] = (uint8_t)headers[i];
    }
    return headers_size;
   }
 */

/*
 * Function: read_headers_payload
 * given a byte array, get the headers payload encoded in it
 * Input: byte_array, frame_header, headers_payload, block_fragment array, padding array
 * padding and dependencies not implemented
 * Output: bytes read or -1 if error
 */
int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
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

/*
 * Function: data_payload_to_bytes
 * Passes a data payload to a byte array
 * Input: frame_header, data_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int data_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    data_payload_t *data_payload = (data_payload_t *) payload;
    int length = frame_header->length;
    uint8_t flags = frame_header->flags;

    if (is_flag_set(flags, DATA_PADDED_FLAG)) {
        //TODO handle padding
        ERROR("Padding not implemented yet");
        return -1;
    }
    int rc = buffer_copy(byte_array, data_payload->data, length);
    if (rc < 0) {
        ERROR("error copying buffer");
        return -1;
    }
    return rc;
}

/*
 * Function: create_data_frame
 * Creates a data frame
 * Input: frame_header, data_payload, data array for data payload, data_to_send(data to save in the data_frame), size of the data to send, stream_id
 * Output: 0 if no error
 */
int create_data_frame(frame_header_t *frame_header, data_payload_t *data_payload, uint8_t *data, uint8_t *data_to_send, int length, uint32_t stream_id)
{
    uint8_t type = DATA_TYPE;
    uint8_t flags = 0x0;

    //uint32_t length = length; //no padding, no dependency. fix if this is implemented

    frame_header->length = length;
    frame_header->type = type;
    frame_header->flags = flags;
    frame_header->stream_id = stream_id;
    frame_header->reserved = 0;
    frame_header->payload_callback = data_payload_to_bytes;

    buffer_copy(data, data_to_send, length);
    data_payload->data = data; //not duplicating info
    return 0;
}


/*
 * Function: read_data_payload
 * given a byte array, get the data payload encoded in it
 * Input: byte_array, frame_header, data_payload, data array
 * Output: byteget_header_block_fragment_sizes read or -1 if error
 */
int read_data_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    data_payload_t *data_payload = (data_payload_t *) payload;
    uint8_t flags = frame_header->flags;
    int length = frame_header->length;

    if (is_flag_set(flags, DATA_PADDED_FLAG)) {
        //TODO handle padding
        ERROR("Padding not implemented yet");
        return -1;
    }
    buffer_copy(data_payload->data, bytes, length);
    return length;
}


/*
 * Function: window_update_payload_to_bytes
 * Passes a window_update payload to a byte array
 * Input: frame_header, window_update_payload pointer, array to save the bytes
 * Output: size of the array of bytes, -1 if error
 */
int window_update_payload_to_bytes(frame_header_t *frame_header, void *payload, uint8_t *byte_array)
{
    window_update_payload_t *window_update_payload = (window_update_payload_t *) payload;
    if (frame_header->length != 4) {
        ERROR("Length != 4, FRAME_SIZE_ERROR");
        return -1;
    }
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
    frame_header->payload_callback = window_update_payload_to_bytes;

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
int read_window_update_payload(frame_header_t *frame_header,void *payload,  uint8_t *buff_read)
{
    window_update_payload_t *window_update_payload = (window_update_payload_t *) payload;
    if (frame_header->length != 4) {
        ERROR("Length != 4, FRAME_SIZE_ERROR");
        return -1;
    }
    uint32_t window_size_increment = bytes_to_uint32_31(buff_read);
    window_update_payload->window_size_increment = window_size_increment;
    return frame_header->length;
}

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
    frame_header->payload_callback = goaway_payload_to_bytes;

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

/*
 * Function: bytes_to_frame_header
 * Transforms bytes to a FrameHeader
 * Input:  array of bytes, size ob bytes to read,pointer to frameheader
 * Output: 0 if bytes were written correctly, -1 if byte size is <9
 */
int bytes_to_frame_header(uint8_t *byte_array, int size, frame_header_t *frame_header)
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

    //TODO: Change this if for switch to implement callbacks in frames

    if(frame_header->type == WINDOW_UPDATE_TYPE){
        frame_header->callback = read_window_update_payload;
    }
    if(frame_header->type == DATA_TYPE){
        frame_header->callback = read_data_payload;
    }
    if(frame_header->type == GOAWAY_TYPE){
        frame_header->callback = read_goaway_payload;
    }
    if(frame_header->type == SETTINGS_TYPE){
        frame_header->callback = bytes_to_settings_payload;
    }
    if(frame_header->type == CONTINUATION_TYPE){
        frame_header->callback = read_continuation_payload;
    }
    if(frame_header->type == HEADERS_TYPE){
        frame_header->callback = read_headers_payload;
    }


    return 0;
}
