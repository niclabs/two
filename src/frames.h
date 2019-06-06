#ifndef FRAMES_H
#define FRAMES_H

#include <stdint.h>
#include "table.h"


/*note the difference between
frameheader (data that identifies a frame of any type) and
headersframe(type of frame that contains http headers)*/

/*FRAME TYPES*/
typedef enum{
    DATA_TYPE = (uint8_t)0x0,
    HEADERS_TYPE = (uint8_t)0x1,
    PRIORITY_TYPE= (uint8_t)0x2,
    RST_STREAM_TYPE = (uint8_t)0x3,
    SETTINGS_TYPE = (uint8_t)0x4,
    PUSH_PROMISE_TYPE = (uint8_t)0x5,
    PING_TYPE= (uint8_t)0x6,
    GOAWAY_TYPE = (uint8_t)0x7,
    WINDOW_UPDATE_TYPE = (uint8_t)0x8,
    CONTINUATION_TYPE = (uint8_t)0x9
}frame_type_t;

/*FRAME HEADER*/
typedef struct{
    uint32_t length:24;
    frame_type_t type;
    uint8_t flags;
    uint8_t reserved:1;
    uint32_t stream_id:31;
}frame_header_t; //72 bits-> 9 bytes

/*FRAME*/
typedef struct{
    frame_header_t* frame_header;
    void * payload;
}frame_t;


/*SETTINGS FRAME*/
typedef struct{
    uint16_t identifier;
    uint32_t value;
}settings_pair_t; //48 bits -> 6 bytes

typedef struct{
    settings_pair_t* pairs;
    int count;
}settings_payload_t; //32 bits -> 4 bytes

typedef enum{
    SETTINGS_ACK_FLAG = 0x1
}setting_flag_t;



/*HEADERS FRAME*/

typedef struct{
    uint8_t pad_length; // only if padded flag is set
    uint8_t exclusive_dependency:1; // only if priority flag is set
    uint32_t stream_dependency:31; // only if priority flag is set
    uint8_t weight; // only if priority flag is set
    uint8_t* header_block_fragment; // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    uint8_t* padding; //only if padded flag is set. Size = pad_length
}headers_payload_t; //48+32+32 bits -> 14 bytes

typedef enum{
    HEADERS_END_STREAM_FLAG = 0x1,//bit 0
    HEADERS_END_HEADERS_FLAG = 0x4,//bit 2
    HEADERS_PADDED_FLAG = 0x8,//bit 3
    HEADERS_PRIORITY_FLAG = 0x20//bit 5
}headers_flag_t;


/*CONTINUATION FRAME*/

typedef struct{
    uint8_t* header_block_fragment; // only if length > 0. Size = frame size
}continuation_payload_t;

typedef enum{
    CONTINUATION_END_HEADERS_FLAG = 0x4//bit 2
}continuation_flag_t;


/*frame header methods*/
int frame_header_to_bytes(frame_header_t* frame_header, uint8_t* byte_array);
int bytes_to_frame_header(uint8_t* byte_array, int size, frame_header_t* frame_header);

int read_headers_payload(uint8_t* read_buffer, frame_header_t* frame_header, headers_payload_t *headers_payload, uint8_t *headers_block_fragment, uint8_t * padding);
int get_header_block_fragment_size(frame_header_t* frame_header, headers_payload_t *headers_payload);
int receive_header_block(uint8_t* header_block_fragments, int header_block_fragments_pointer, table_pair_t* header_list, uint8_t table_index);

/*frame continuation methods*/
int read_continuation_payload(uint8_t* buff_read, frame_header_t* frame_header, continuation_payload_t* continuation_payload, uint8_t * continuation_block_fragment);


/*flags methods*/
int is_flag_set(uint8_t flags, uint8_t flag);
uint8_t set_flag(uint8_t flags, uint8_t flag_to_set);

/*frame methods*/
int frame_to_bytes(frame_t* frame, uint8_t* bytes);
//int bytesToFrame(uint8_t * bytes, int size, frame_t* frame);

/*settings methods*/
int create_list_of_settings_pair(uint16_t* ids, uint32_t* values, int count, settings_pair_t* pair_list);
int create_settings_frame(uint16_t* ids, uint32_t* values, int count, frame_t* frame, frame_header_t* frame_header, settings_payload_t* settings_payload, settings_pair_t* pairs);
int setting_to_bytes(settings_pair_t* setting, uint8_t* byte_array);
int settings_frame_to_bytes(settings_payload_t* settings_frame, uint32_t count, uint8_t* byte_array);

int bytes_to_settings_payload(uint8_t* bytes, int size, settings_payload_t* settings_frame, settings_pair_t* pairs);

int create_settings_ack_frame(frame_t * frame, frame_header_t* frame_header);




/*header payload methods*/
int create_headers_frame(uint8_t * headers_block, int headers_block_size, uint32_t stream_id, frame_t * frame, frame_header_t* frame_header, headers_payload_t* headers_payload);

/*continuation payload methods*/
int create_continuation_frame(uint8_t * headers_block, int headers_block_size, uint32_t stream_id, frame_t * frame, frame_header_t* frame_header, continuation_payload_t* continuation_payload);


//TODO

/*Headers compresion*/
//int compress_headers(char* headers, int headers_size, uint8_t* compressed_headers, int compressed_headers_size);
//int compress_headers_with_strategy(char* headers, int headers_size, uint8_t* compressed_headers, int compressed_headers_size, uint8_t bool_table_compression, uint8_t bool_huffman_compression);



/*
void* byteToPayloadDispatcher[10];
byteToPayloadDispatcher[SETTINGS_TYPE] = &bytesToSettingsPayload;


typedef* payloadTypeDispatcher[10];
payloadTypeDispatcher[SETTINGS_TYPE] = &settings_payload_t
*/

#endif /*FRAMES_H*/
