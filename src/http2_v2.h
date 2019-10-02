#ifndef HTTP2_V2_H
#define HTTP2_V2_H

#define HTTP2_MAX_HBF_BUFFER 16384

typedef enum {
    STREAM_IDLE,
    STREAM_OPEN,
    STREAM_HALF_CLOSED_REMOTE,
    STREAM_HALF_CLOSED_LOCAL,
    STREAM_CLOSED
} h2_stream_state_t;

typedef struct HTTP2_STREAM {
    uint32_t stream_id;
    h2_stream_state_t state;
} h2_stream_t;

typedef struct HTTP2_WINDOW_MANAGER {
    uint32_t window_size;
    uint32_t window_used;
} h2_window_manager_t;


/*Struct for storing HTTP2 states*/
typedef struct HTTP2_STATES {
    uint32_t remote_settings[6];
    uint32_t local_settings[6];
    /*uint32_t local_cache[6]; Could be implemented*/
    //uint8_t wait_setting_ack;
    h2_stream_t current_stream;
    uint32_t last_open_stream_id;
    uint8_t header_block_fragments[HTTP2_MAX_HBF_BUFFER];
    uint8_t header_block_fragments_pointer; //points to the next byte to write in
    //uint8_t waiting_for_end_headers_flag;   //bool
    //uint8_t received_end_stream;
    h2_window_manager_t incoming_window;
    h2_window_manager_t outgoing_window;
    //uint8_t sent_goaway;
    //uint8_t received_goaway;        // bool
    uint8_t debug_data_buffer[0];   // TODO not implemented yet
    uint8_t debug_size;             // TODO not implemented yet
    //Hpack dynamic table
    hpack_states_t hpack_states;
} h2states_t;

typedef struct CALLBACK {
  struct CALLBACK (* func)(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
  void* debug_info; // just in case
} callback_t ;

callback_t h2_server_init_connection(cbuf_t* buf_in, cbuf_t* buf_out, void* state);
