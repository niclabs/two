#ifndef HTTP2_STRUCTS_H
#define HTTP2_STRUCTS_H

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "event.h"
#include "headers.h"
#include "frames/frames.h"
#include "config.h"

/*Macros for table update*/
// TODO: rename LOCAL,REMOTE constants to LOCAL_SETTINGS, REMOTE_SETTINGS
#define LOCAL 0
#define REMOTE 1


/*Definition of max headers frame buffer size*/
#ifdef CONFIG_MAX_HBF_BUFFER
#define HTTP2_MAX_HBF_BUFFER (CONFIG_MAX_HBF_BUFFER)
#else
#define HTTP2_MAX_HBF_BUFFER 256
#endif

/*Definition of max buffer size*/
#ifdef CONFIG_MAX_BUFFER_SIZE
#define HTTP2_MAX_BUFFER_SIZE (CONFIG_MAX_BUFFER_SIZE)
#else
#define HTTP2_MAX_BUFFER_SIZE 256
#endif


/*Default settings values*/
#define DEFAULT_HEADER_TABLE_SIZE 4096
#define DEFAULT_ENABLE_PUSH 0
#define DEFAULT_MAX_CONCURRENT_STREAMS 1
#define DEFAULT_INITIAL_WINDOW_SIZE 65535
#define DEFAULT_MAX_FRAME_SIZE 16384
#define DEFAULT_MAX_HEADER_LIST_SIZE 2147483647

#define SET_FLAG(flag_bits, flag_type)   \
    (flag_bits |= 1 << flag_type)

#define CLEAR_FLAG(flag_bits, flag_type)   \
    (flag_bits &= ~ (1 << flag_type))

#define FLAG_VALUE(flag_bits, flag_type)       \
    ((flag_bits >> flag_type) & 1)

/*Error codes*/
typedef enum __attribute__((__packed__)){
    HTTP2_NO_ERROR              = (uint32_t) 0x0,
    HTTP2_PROTOCOL_ERROR        = (uint32_t) 0x1,
    HTTP2_INTERNAL_ERROR        = (uint32_t) 0x2,
    HTTP2_FLOW_CONTROL_ERROR    = (uint32_t) 0x3,
    HTTP2_SETTINGS_TIMEOUT      = (uint32_t) 0x4,
    HTTP2_STREAM_CLOSED         = (uint32_t) 0x5,
    HTTP2_FRAME_SIZE_ERROR      = (uint32_t) 0x6,
    HTTP2_REFUSED_STREAM        = (uint32_t) 0x7,
    HTTP2_CANCEL                = (uint32_t) 0x8,
    HTTP2_COMPRESSION_ERROR     = (uint32_t) 0x9,
    HTTP2_CONNECT_ERROR         = (uint32_t) 0xa,
    HTTP2_ENHANCE_YOUR_CALM     = (uint32_t) 0xb,
    HTTP2_INADEQUATE_SECURITY   = (uint32_t) 0xc,
    HTTP2_HTTP_1_1_REQUIRED     = (uint32_t) 0xd
}h2_error_code_t;

/*Enumerator for stream states*/
typedef enum __attribute__((__packed__)){
    STREAM_IDLE,
    STREAM_OPEN,
    STREAM_HALF_CLOSED_REMOTE,
    STREAM_HALF_CLOSED_LOCAL,
    STREAM_CLOSED
} h2_stream_state_t;

/*Enumerator for settings parameters*/
typedef enum __attribute__((__packed__)) SettingsParameters {
    HEADER_TABLE_SIZE       = (uint16_t) 0x1,
    ENABLE_PUSH             = (uint16_t) 0x2,
    MAX_CONCURRENT_STREAMS  = (uint16_t) 0x3,
    INITIAL_WINDOW_SIZE     = (uint16_t) 0x4,
    MAX_FRAME_SIZE          = (uint16_t) 0x5,
    MAX_HEADER_LIST_SIZE    = (uint16_t) 0x6
}sett_param_t;

typedef enum __attribute__((__packed__)){
    HTTP2_RC_CLOSE_CONNECTION               = 2,
    HTTP2_RC_ACK_RECEIVED                   = 1,
    HTTP2_RC_NO_ERROR                       = 0,
    HTTP2_RC_ERROR                          = -1,
    HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT    = -2,
} h2_ret_code_t;


typedef enum __attribute__((__packed__)){
    FLAG_IS_SERVER = 0,
    FLAG_WAIT_SETTINGS_ACK = 1,
    FLAG_WAITING_FOR_END_HEADERS_FLAG = 2,
    FLAG_WAITING_FOR_HEADERS_FRAME = 3,
    FLAG_RECEIVED_END_STREAM = 4,
    FLAG_WRITE_CALLBACK_IS_SET = 5,
    FLAG_SENT_GOAWAY = 6,
    FLAG_RECEIVED_GOAWAY = 7,
} h2_flags_t;


#pragma pack(push, 1)
/*Struct for HTTP2 Stream*/
typedef struct HTTP2_STREAM {
    uint32_t stream_id;
    h2_stream_state_t state;
} h2_stream_t;

typedef struct HTTP2_FLOW_CONTROL_WINDOW {
    int connection_window;
    int stream_window;
} h2_flow_control_window_t;

typedef struct HTTP2_DATA {
    uint32_t size;
    uint8_t *buf;
    uint32_t processed;
} http2_data_t;


/*Struct for storing HTTP2 states*/
typedef struct http2_states {
    // make a http2_states a linked list
    // IMPORTANT: this pointer must be the first of the struct
    // in order to use the list macros
    struct http2_states *next;

    event_sock_t *socket;
    uint8_t flag_bits;

    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/

    h2_stream_t current_stream;
    uint32_t last_open_stream_id;
    uint8_t header_block_fragments[HTTP2_MAX_HBF_BUFFER];
    uint32_t header_block_fragments_pointer;     //points to the next byte to write in
    h2_flow_control_window_t remote_window;
    h2_flow_control_window_t local_window;

    //Hpack dynamic table
    hpack_states_t hpack_states;

    // input buffer for event_read
    uint8_t input_buf[HTTP2_MAX_BUFFER_SIZE];

    frame_header_t header;
    http2_data_t data;
    header_list_t headers;
} h2states_t;

#pragma pack(pop)

#endif
