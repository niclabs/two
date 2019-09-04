#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2.h"
#include "http2utils.h"
#include "http2_flowcontrol.h"
//#define LOG_LEVEL LOG_LEVEL_INFO
#include "logging.h"


/*---------- Import of functions not declared in http2.h ----------------*/
extern int init_variables(hstates_t * st);
extern uint32_t read_setting_from(hstates_t *st, uint8_t place, uint8_t param);
extern int send_local_settings(hstates_t *st);
extern int update_settings_table(settings_payload_t *spl, uint8_t place, hstates_t *st);
extern int send_settings_ack(hstates_t *st);
extern int check_incoming_settings_condition(frame_header_t *header, hstates_t *st);
extern int handle_settings_payload(settings_payload_t *spl, hstates_t *st);
extern int read_frame(uint8_t *buff_read, frame_header_t *header, hstates_t *st);
extern int check_incoming_headers_condition(frame_header_t *header, hstates_t *st);
extern int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, hstates_t *st);
extern int check_incoming_continuation_condition(frame_header_t *header, hstates_t *st);
extern int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, hstates_t *st);
extern int send_headers_stream_verification(hstates_t *st);
extern int send_headers_frame(hstates_t *st, uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, uint8_t end_stream);
extern int send_continuation_frame(hstates_t *st, uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_stream);
extern int send_headers(hstates_t *st, uint8_t end_stream);
extern int handle_data_payload(frame_header_t* frame_header, data_payload_t* data_payload, hstates_t* st);
extern int send_data(hstates_t *st, uint8_t end_stream);
extern int send_window_update(hstates_t *st, uint8_t window_size_increment);
extern int prepare_new_stream(hstates_t *st);
extern int change_stream_state_end_stream_flag(hstates_t *st, uint8_t sending);
extern int send_goaway(hstates_t *st, uint32_t error_code);


 /*---------------- Mock functions ---------------------------*/

 uint8_t buffer[HTTP2_MAX_BUFFER_SIZE];
 int size = 0;

 // Toy write function
int http_write(hstates_t *st, uint8_t *bytes, int length){
 if(size+length > HTTP2_MAX_BUFFER_SIZE){
   return -1;
 }
 memcpy(buffer+size, bytes, length);
 size += length;
 return length;
}

// Toy read function
int http_read(hstates_t *st, uint8_t *bytes, int length){
  if(length > size){
      length = size;
  }
  //Write to caller
  memcpy(bytes, buffer, length);
  size = size - length;
  //Move the rest of the data on buffer
  memcpy(buffer, buffer+length, size);
  return length;
}

int read_n_bytes(uint8_t *buff_read, int n, hstates_t *st){
 int read_bytes = 0;
 int incoming_bytes;
 while(read_bytes < n){
   incoming_bytes = http_read(st, buff_read+read_bytes, n - read_bytes);
   /* incoming_bytes equals -1 means that there was an error*/
   if(incoming_bytes < 1){
     return -1;
   }
   read_bytes = read_bytes + incoming_bytes;
 }
 return read_bytes;
}

int clean_buffer(void){
  size = 0;
  return 0;
}


int headers_init_custom_fake(headers_t *headers, header_t *hlist, int maxlen)
{
  memset(headers, 0, sizeof(*headers));
  memset(hlist, 0, maxlen * sizeof(*hlist));
  headers->headers = hlist;
  headers->maxlen = maxlen;
  headers->count = 0;
  return 0;
}


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, create_settings_ack_frame, frame_t *, frame_header_t*);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t*, uint8_t*);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, bytes_to_settings_payload, uint8_t*, int, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, bytes_to_frame_header, uint8_t*, int , frame_header_t*);
FAKE_VALUE_FUNC(int, create_settings_frame ,uint16_t*, uint32_t*, int, frame_t*, frame_header_t*, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t*, uint8_t*, int);
FAKE_VALUE_FUNC(uint32_t, get_header_block_fragment_size,frame_header_t*, headers_payload_t*);
FAKE_VALUE_FUNC(int, receive_header_block,uint8_t*, int, headers_t*, hpack_dynamic_table_t*);
FAKE_VALUE_FUNC(int, read_headers_payload, uint8_t*, frame_header_t*, headers_payload_t*, uint8_t*, uint8_t*);//TODO fix this
FAKE_VALUE_FUNC(int, read_continuation_payload, uint8_t*, frame_header_t*, continuation_payload_t*, uint8_t*);//TODO fix this
FAKE_VALUE_FUNC(uint32_t, get_setting_value, uint32_t*, sett_param_t);
FAKE_VALUE_FUNC(uint32_t, headers_get_header_list_size, headers_t*);


FAKE_VALUE_FUNC( int, compress_headers, headers_t* , uint8_t*, hpack_dynamic_table_t*);
FAKE_VALUE_FUNC( int, create_headers_frame, uint8_t * , int , uint32_t , frame_header_t* , headers_payload_t* ,uint8_t*);
FAKE_VALUE_FUNC( int, headers_payload_to_bytes, frame_header_t* , headers_payload_t* , uint8_t* );
FAKE_VALUE_FUNC( int, create_continuation_frame, uint8_t * , int , uint32_t , frame_header_t* , continuation_payload_t* , uint8_t*);
FAKE_VALUE_FUNC( int, continuation_payload_to_bytes, frame_header_t* , continuation_payload_t* , uint8_t* );
FAKE_VALUE_FUNC( uint8_t, set_flag, uint8_t , uint8_t );

FAKE_VALUE_FUNC(int, create_data_frame, frame_header_t* , data_payload_t* , uint8_t * , uint8_t * , int , uint32_t );
FAKE_VALUE_FUNC(int, data_payload_to_bytes, frame_header_t* , data_payload_t* , uint8_t*);
FAKE_VALUE_FUNC(int, read_data_payload, uint8_t* , frame_header_t* , data_payload_t* , uint8_t *);
FAKE_VALUE_FUNC(int, create_window_update_frame, frame_header_t* , window_update_payload_t* , int , uint32_t );
FAKE_VALUE_FUNC(int, window_update_payload_to_bytes, frame_header_t* , window_update_payload_t* , uint8_t* );
FAKE_VALUE_FUNC(int, read_window_update_payload, uint8_t* , frame_header_t* , window_update_payload_t* );

FAKE_VALUE_FUNC(int, create_goaway_frame, frame_header_t*, goaway_payload_t*, uint8_t*, uint32_t, uint32_t,  uint8_t *, uint8_t);
FAKE_VALUE_FUNC(int, goaway_payload_to_bytes, frame_header_t*, goaway_payload_t*, uint8_t*);
FAKE_VALUE_FUNC(int, read_goaway_payload, uint8_t*, frame_header_t*, goaway_payload_t* , uint8_t*);

FAKE_VALUE_FUNC(uint32_t, get_window_available_size, h2_window_manager_t);
FAKE_VALUE_FUNC(int, flow_control_send_data, hstates_t*, uint32_t);
FAKE_VALUE_FUNC(int, flow_control_receive_data, hstates_t*, uint32_t);
FAKE_VALUE_FUNC(int, flow_control_send_window_update, hstates_t*, uint32_t);
FAKE_VALUE_FUNC(int, flow_control_receive_window_update, hstates_t*, uint32_t);

FAKE_VALUE_FUNC(int, prepare_new_stream, hstates_t*);
FAKE_VALUE_FUNC(uint32_t, read_setting_from, hstates_t*, uint8_t, uint8_t);
FAKE_VALUE_FUNC(uint32_t, get_size_data_to_send, hstates_t*);

FAKE_VALUE_FUNC(int8_t, hpack_init_dynamic_table, hpack_dynamic_table_t *);

#define FFF_FAKES_LIST(FAKE)              \
    FAKE(create_settings_ack_frame)       \
    FAKE(frame_to_bytes)                  \
    FAKE(is_flag_set)                     \
    FAKE(bytes_to_settings_payload)       \
    FAKE(bytes_to_frame_header)           \
    FAKE(create_settings_frame)           \
    FAKE(buffer_copy)                     \
    FAKE(get_header_block_fragment_size)  \
    FAKE(receive_header_block)            \
    FAKE(read_headers_payload)            \
    FAKE(read_continuation_payload)    \
    FAKE(get_setting_value)               \
    FAKE(headers_get_header_list_size)            \
    FAKE(compress_headers)            \
    FAKE(headers_payload_to_bytes)            \
    FAKE(create_headers_frame)            \
    FAKE(create_continuation_frame)            \
    FAKE(continuation_payload_to_bytes)            \
    FAKE(set_flag)            \
    FAKE(read_data_payload)             \
    FAKE(read_window_update_payload)    \
    FAKE(create_data_frame)           \
    FAKE(data_payload_to_bytes)           \
    FAKE(create_window_update_frame)           \
    FAKE(window_update_payload_to_bytes)           \
    FAKE(create_goaway_frame)   \
    FAKE(goaway_payload_to_bytes)   \
    FAKE(read_goaway_payload)   \
    FAKE(get_window_available_size)           \
    FAKE(flow_control_send_data)   \
    FAKE(flow_control_receive_data)   \
    FAKE(flow_control_send_window_update)   \
    FAKE(flow_control_receive_window_update)   \
    FAKE(prepare_new_stream) \
    FAKE(read_setting_from) \
    FAKE(get_size_data_to_send) \
    FAKE(hpack_init_dynamic_table)  \

/*----------Value Return for FAKEs ----------*/
int create_ack_return_zero(frame_t * f, frame_header_t* fh){
  return 0;
}
int create_return_zero(uint16_t* u, uint32_t* uu, int uuu, frame_t *f , frame_header_t *fh, settings_payload_t *sp, settings_pair_t* spp){
  return 0;
}
int frame_bytes_return_9(frame_t *f, uint8_t *u){
  return 9;
}
int frame_bytes_return_24(frame_t *f, uint8_t *u){
  return 24;
}
int frame_bytes_return_45(frame_t *f, uint8_t *u){
  return 45;
}
int bytes_frame_return_zero(uint8_t* u, int uu, frame_header_t *fh){
  return 0;
}
int bytes_settings_payload_return_24(uint8_t*u, int uu, settings_payload_t* sp, settings_pair_t* spp){
  return 24;
}

int buffer_copy_fake_custom(uint8_t* dest, uint8_t* orig, int size){
  for(int i = 0; i< size; i++){
    dest[i] = orig[i];
  }
  return size;
}
//getheaderblockfragmentsize
uint32_t ghbfs(frame_header_t *h, headers_payload_t *st){
  return 20;
}
int bc(uint8_t* a, uint8_t*b, int c){
  return 20;
}


/*---------- TESTING SUIT --------------------*/
void setUp(void){
  /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
  /*Reset the size of fake buffer*/
  size = 0;
}

void test_init_variables(void){
  hstates_t hdummy;
  hdummy.is_server = 1;
  uint32_t init_vals[6] = {DEFAULT_HEADER_TABLE_SIZE,DEFAULT_ENABLE_PUSH,DEFAULT_MAX_CONCURRENT_STREAMS,DEFAULT_INITIAL_WINDOW_SIZE,DEFAULT_MAX_FRAME_SIZE,DEFAULT_MAX_HEADER_LIST_SIZE};
  int rc = init_variables(&hdummy);
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 0, "WAIT must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.current_stream.stream_id == 2, "Current stream id must be 2");
  TEST_ASSERT_MESSAGE(hdummy.h2s.current_stream.state == STREAM_IDLE, "Current stream state must be IDLE");
  TEST_ASSERT_MESSAGE(hdummy.h2s.last_open_stream_id == 1, "Last open stream id must be 1");
  TEST_ASSERT_MESSAGE(hdummy.h2s.incoming_window.window_size == DEFAULT_INITIAL_WINDOW_SIZE, "window size must be DEFAULT_INITIAL_WINDOW_SIZE");
  TEST_ASSERT_MESSAGE(hdummy.h2s.incoming_window.window_used == 0, "window used must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.outgoing_window.window_size == DEFAULT_INITIAL_WINDOW_SIZE, "window size must be DEFAULT_INITIAL_WINDOW_SIZE");
  TEST_ASSERT_MESSAGE(hdummy.h2s.outgoing_window.window_used == 0, "window used must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.sent_goaway == 0, "sent go away must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.received_goaway == 0, "received go away must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.debug_size == 0, "debug_size must be 0");

  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
}

void test_update_settings_table(void){
  settings_pair_t pair1 = {0x1, 12345};
  settings_pair_t pair2 = {0x2, 0};
  settings_pair_t pair3 = {0x3, 12345};
  settings_pair_t pair4 = {0x4, 12345};
  settings_pair_t pair5 = {0x5, 17345};
  settings_pair_t pair6 = {0x6, 12345};
  settings_pair_t pairs[6] = {pair1,pair2,pair3,pair4,pair5,pair6};
  settings_payload_t payload = {pairs, 6};
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  int rc = update_settings_table(&payload, LOCAL, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[0] == 12345, "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[1] == 0, "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[2] == 12345, "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[3] == 12345, "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[4] == 17345, "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[5] == 12345, "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[0] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[1] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == 1, "Remote settings were not modified!");
  rc = update_settings_table(&payload, REMOTE, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[0] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[1] == 0, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[2] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[3] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[4] == 17345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[5] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[0] == 12345, "HTS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[1] == 0, "EP in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 12345, "MCS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 12345, "IWS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 17345, "MFS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == 12345, "MHLS in remote settings must be modified!");

}

void test_update_settings_table_errors(void){
  settings_pair_t pair1 = {0x1, 12345};
  settings_pair_t pair2 = {0x2, 0};
  settings_pair_t pair3 = {0x3, 12345};
  settings_pair_t pair4 = {0x4, 12345};
  settings_pair_t pair5 = {0x5, 12345};
  settings_pair_t pair6 = {0x6, 12345};
  settings_pair_t pairs[6] = {pair1,pair2,pair3,pair4,pair5,pair6};
  settings_payload_t payload = {pairs, 6};
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  int rc = update_settings_table(&payload, LOCAL, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1, invalid setting inserted");
  rc = update_settings_table(&payload, 5, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, not valid table");
}

void test_send_settings_ack(void){
  hstates_t hdummy;
  create_settings_ack_frame_fake.custom_fake = create_ack_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_9;
  int rc = send_settings_ack(&hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count = 1, "ACK creation not called");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count = 1, "frame to bytes not called");
  TEST_ASSERT_MESSAGE(size == 9, "9 bytes must be written by send_settings_ack");
}

void test_send_settings_ack_errors(void){
  hstates_t hdummy;
  int create_return[2] = {-1, 0};
  SET_RETURN_SEQ(create_settings_ack_frame, create_return, 2);
  int frame_to_bytes_return[1] = {10000};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_settings_ack(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in creation)");
  TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count = 1, "ACK creation not called");
  rc = send_settings_ack(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in settings write)");
}

void test_check_incoming_settings_condition(void){
  hstates_t hdummy;
  init_variables(&hdummy);
  hdummy.h2s.wait_setting_ack = 1;
  frame_header_t header_ack = {0, 0x4, 0x0|0x1, 0x0, 0};
  frame_header_t header_not_ack = {24, 0x4, 0x0, 0x0, 0};
  int flag_returns[3] = {0, 1, 1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 3);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_settings_condition(&header_not_ack, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0. ACK flag is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 1, "wait must remain in 1");
  rc = check_incoming_settings_condition(&header_ack, &hdummy);
  TEST_ASSERT_MESSAGE(is_flag_set_fake.call_count == 2, "is flag set must be called for second time");
  TEST_ASSERT_MESSAGE(rc == 1, "RC must be 1. ACK flag was setted and payload size was 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 0, "wait must be changed to 0");
  rc = check_incoming_settings_condition(&header_ack, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 1, "RC must be 1. ACK flag was setted, but not wait ack flag asigned");
}

void test_check_incoming_settings_condition_errors(void){
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  hdummy.h2s.wait_setting_ack = 1;
  // second error, wrong stream_id
  frame_header_t header_ack_wrong_stream = {0, 0x4, 0x0|0x1, 0x1, 0};
  header_ack_wrong_stream.stream_id = 1;
  // third error, wrong size
  frame_header_t header_ack_wrong_size = {24, 0x4, 0x0|0x1, 0x0, 0};
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  int rc = check_incoming_settings_condition(&header_ack_wrong_stream, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (wrong stream)");
  rc = check_incoming_settings_condition(&header_ack_wrong_size, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (wrong size)");
}

void test_handle_settings_payload(void){
  settings_pair_t pair1 = {0x3, 12345};
  settings_pair_t pair2 = {0x4, 12345};
  settings_pair_t pair3 = {0x5, 17345};
  settings_pair_t pair4 = {0x6, 12345};
  settings_pair_t pairs[4] = {pair1,pair2,pair3,pair4};
  settings_payload_t payload = {pairs, 4};
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  bytes_to_settings_payload_fake.custom_fake = bytes_settings_payload_return_24;
  create_settings_ack_frame_fake.custom_fake = create_ack_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_9;
  int rc = handle_settings_payload(&payload, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 0, "ACK wait must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 12345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 12345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 17345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == 12345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(size == 9, "Expected 9 bytes on buffer");
}

void test_handle_settings_payload_errors(void){
  settings_pair_t pair1 = {0x3, 12345};
  settings_pair_t pair2 = {0x4, 12345};
  settings_pair_t pair3 = {0x5, 12345};
  settings_pair_t pair4 = {0x6, 12345};
  settings_pair_t pairs[4] = {pair1,pair2,pair3,pair4};
  settings_payload_t payload = {pairs, 4};
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  create_settings_ack_frame_fake.custom_fake = create_ack_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_9;
  int rc = handle_settings_payload(&payload, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in update settings table)");
}

void test_read_frame(void){
  hstates_t hst;
  init_variables(&hst);
  frame_header_t header = {36, 0x4, 0x0, 0x0, 0};
  uint8_t bf[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  bytes_to_frame_header_fake.custom_fake = bytes_frame_return_zero;
  /*We write 200 zeros for future reading*/
  uint32_t read_setting_from_returns[1] = {1024};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int wrc = http_write(&hst, bf, 200);
  TEST_ASSERT_MESSAGE(wrc == 200, "wrc must be 200.");
  TEST_ASSERT_MESSAGE(size == 200, "size must be 200.");
  int rc = read_frame(bf, &header, &hst);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(size == 155, "read_frame must have read 45 bytes");
  TEST_ASSERT_MESSAGE(bytes_to_frame_header_fake.call_count == 1, "bytes to frame header must be called once");
}

void test_read_frame_errors(void){
  hstates_t hst;
  init_variables(&hst);
  /*Header with payload greater than 1024*/
  frame_header_t bad_header = {2048, 0x4, 0x0, 0x0, 0};
  frame_header_t good_header = {200, 0x4, 0x0, 0x0, 0};
  uint8_t bf[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  // Second error, bytes_to_frame_header return
  int bytes_return[2] = {-1, 0};
  SET_RETURN_SEQ(bytes_to_frame_header, bytes_return, 2);
  uint32_t read_setting_from_returns[1] = {1024};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int frame_to_bytes_return[1] = {1000};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  // First error, there is no data to read
  int rc = read_frame(bf, &good_header, &hst);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (read_n_bytes error)");
  http_write(&hst, bf, 9);
  // Second error
  rc = read_frame(bf, &good_header, &hst);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (bytes_to_frame_header error)");
  /*We write 200 zeros for future reading*/
  // Third error, payload size too big
  rc = read_frame(bf, &bad_header, &hst);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (payload size error)");
  TEST_ASSERT_MESSAGE(bytes_to_frame_header_fake.call_count == 2, "bytes to frame header must be called once");
  // Fourth error, read_n_bytes payload reading error
  clean_buffer();
  rc = read_frame(bf, &good_header, &hst);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (payload read error)");
}

void test_send_local_settings(void){
  /*Depends on create_settings_frame, frame_to_bytes and http_write*/
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  int rc = send_local_settings(&hdummy);
  TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame_to_bytes must be called once");
  TEST_ASSERT_MESSAGE(size == 45, "send local settings must have written 45 bytes");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 1, "wait_setting_ack must be 1. Settings were sent");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of send_local_settings must be 0");
}

void test_send_local_settings_errors(void){
  /*Depends on create_settings_frame, frame_to_bytes and http_write*/
  hstates_t hdummy;
  init_variables(&hdummy);
  int i;
  for(i = 0; i < 6; i++){
      hdummy.h2s.local_settings[i] = 1;
      hdummy.h2s.remote_settings[i] = 1;
  }
  // First error, create settings frame error
  int create_return[2] = {-1, 0};
  SET_RETURN_SEQ(create_settings_frame, create_return, 2);
  // Second error, http_write error
  int frame_return[3] = {17000, 17000, 12};
  SET_RETURN_SEQ(frame_to_bytes, frame_return, 3);
  int rc = send_local_settings(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (create_settings_frame error)");
  rc = send_local_settings(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (http_write error)");
}

void test_h2_client_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  hstates_t client;
  uint32_t init_vals[6] = {DEFAULT_HEADER_TABLE_SIZE,DEFAULT_ENABLE_PUSH,DEFAULT_MAX_CONCURRENT_STREAMS,DEFAULT_INITIAL_WINDOW_SIZE,DEFAULT_MAX_FRAME_SIZE,DEFAULT_MAX_HEADER_LIST_SIZE};
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  int rc = h2_client_init_connection(&client);
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[0] == init_vals[0], "HTS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[1] == init_vals[1], "EP in remote settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[2] == init_vals[2], "MCS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[3] == init_vals[3], "IWS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[4] == init_vals[4], "MFS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(client.h2s.remote_settings[5] == init_vals[5], "MHLS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(size == 69, "client must wrote 69 bytes, 45 of settings, 24 of preface");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of h2_client_init_connection must be 0");
}

void test_h2_client_init_connection_errors(void){
  /*Depends on http_write and send_local_settings*/
  hstates_t client;
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  uint8_t bf[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  // Fill the buffer, so http_write must return an error
  int wrc = http_write(&client, bf, HTTP2_MAX_BUFFER_SIZE);
  (void) wrc;
  // First error, http_write
  int rc = h2_client_init_connection(&client);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (http_write error)");
  // Empty 24 bytes on the buffer, so writting is possible.
  wrc = http_read(&client, bf, 24);
  // Error in create settings frame error, so send_local_settings fails
  int create_return[1] = {-1};
  SET_RETURN_SEQ(create_settings_frame, create_return, 1);
  rc = h2_client_init_connection(&client);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (send_local_settings error)");
}


void test_h2_server_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  hstates_t server;
  uint32_t init_vals[6] = {DEFAULT_HEADER_TABLE_SIZE,DEFAULT_ENABLE_PUSH,DEFAULT_MAX_CONCURRENT_STREAMS,DEFAULT_INITIAL_WINDOW_SIZE,DEFAULT_MAX_FRAME_SIZE,DEFAULT_MAX_HEADER_LIST_SIZE};
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  uint8_t preface_buff[24];
  uint8_t i = 0;
  /*We load the fake buffer with the preface*/
  while(preface[i] != '\0'){
    preface_buff[i] = preface[i];
    i++;
  }
  int wrc = http_write(&server, preface_buff,24);
  TEST_ASSERT_MESSAGE(wrc == 24, "Fake buffer not written");
  TEST_ASSERT_MESSAGE(size == 24, "Fake buffer not written");
  int rc = h2_server_init_connection(&server);
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[0] == init_vals[0], "HTS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[1] == init_vals[1], "EP in remote settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[2] == init_vals[2], "MCS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[3] == init_vals[3], "IWS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[4] == init_vals[4], "MFS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(server.h2s.remote_settings[5] == init_vals[5], "MHLS in remote settings is not setted");
  TEST_ASSERT_MESSAGE(size == 45, "server must have written 45 bytes of settings");
  TEST_ASSERT_MESSAGE(rc==0, "return code of h2_server_init_connection must be 0");
}

void test_h2_server_init_connection_errors(void){
  /*Depends on http_write and send_local_settings*/
  hstates_t server;
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  // First error, we read and no preface is found inside buffer
  int rc = h2_server_init_connection(&server);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (read_n_bytes error)");
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  uint8_t preface_buff[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  // Second error, garbage was written before the correct string
  int wrc = http_write(&server, preface_buff, 1);
  uint8_t i = 0;
  /*We load the fake buffer with the preface*/
  while(preface[i] != '\0'){
    preface_buff[i] = preface[i];
    i++;
  }
  wrc = http_write(&server, preface_buff,24);
  TEST_ASSERT_MESSAGE(wrc == 24, "Fake buffer not written");
  rc = h2_server_init_connection(&server);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (strcmp error (PRI *... was not found))");
  rc = clean_buffer();
  // Writting the correct preface
  wrc = http_write(&server, preface_buff,24);
  TEST_ASSERT_MESSAGE(wrc == 24, "Fake buffer not written");
  wrc = http_write(&server, preface_buff, HTTP2_MAX_BUFFER_SIZE - 24);
  TEST_ASSERT_MESSAGE(wrc == HTTP2_MAX_BUFFER_SIZE - 24, "Fake buffer not written (MAX BUFFER)");
  rc = h2_server_init_connection(&server);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (send_local_settings error)");

}

void test_check_incoming_headers_condition(void){
  frame_header_t head;
  head.stream_id = 2440;
  head.length = 128;
  hstates_t st;
  st.is_server = 0;
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.stream_id = 2440;
  st.h2s.current_stream.state = STREAM_OPEN;
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

}

void test_check_incoming_headers_condition_error(void){
  hstates_t st_end_flag;
  frame_header_t head;
  st_end_flag.h2s.waiting_for_end_headers_flag = 1;
  hstates_t st_valid;
  frame_header_t head_invalid;
  head_invalid.stream_id = 0;
  st_valid.h2s.waiting_for_end_headers_flag = 0;
  hstates_t st_last;
  frame_header_t head_ngt;
  st_last.h2s.waiting_for_end_headers_flag = 0;
  st_last.h2s.last_open_stream_id = 124;
  head_ngt.stream_id = 123;
  head_ngt.length = 100;
  hstates_t st_parity;
  frame_header_t head_parity;
  st_parity.is_server = 1;
  st_parity.h2s.waiting_for_end_headers_flag = 0;
  st_parity.h2s.last_open_stream_id = 2;
  head_parity.stream_id = 120;
  head_parity.length = 100;
  hstates_t st_parity_c;
  frame_header_t head_parity_c;
  st_parity.is_server = 0;
  st_parity.h2s.waiting_for_end_headers_flag = 0;
  st_parity.h2s.last_open_stream_id = 3;
  head_parity.stream_id = 17;
  hstates_t st_fs;
  frame_header_t head_fs;
  st_fs.h2s.waiting_for_end_headers_flag = 0;
  head_fs.stream_id = 12;
  head_fs.length = 200;
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_headers_condition(&head, &st_end_flag);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (waiting for end headers flag set)");
  rc = check_incoming_headers_condition(&head_invalid, &st_valid);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id equals to 0)");
  rc = check_incoming_headers_condition(&head_ngt, &st_last);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id not bigger than last open)");
  rc = check_incoming_headers_condition(&head_parity, &st_parity);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id parity is wrong)");
  rc = check_incoming_headers_condition(&head_parity_c, &st_parity_c);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id parity is wrong)");
  rc = check_incoming_headers_condition(&head_fs, &st_fs);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (frame size bigger than expected)");
}

void test_check_incoming_headers_condition_creation_of_stream(void){
  frame_header_t head;
  head.stream_id = 2440;
  head.length = 128;
  hstates_t st;
  st.is_server = 0;
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.stream_id = 0;
  st.h2s.current_stream.state = 0;
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
  st.h2s.current_stream.stream_id = 2438;
  st.h2s.current_stream.state = STREAM_IDLE;
  rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

}

void test_check_incoming_headers_condition_mismatch(void){
  frame_header_t head;
  head.stream_id = 2440;
  head.length = 256;
  uint32_t read_setting_from_returns[1] = {280};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  hstates_t st;
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.stream_id = 2438;
  st.h2s.current_stream.state = STREAM_OPEN;
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2438, "Stream id must be 2438");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
  st.h2s.current_stream.stream_id = 2440;
  st.h2s.current_stream.state = STREAM_CLOSED;
  rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
}

void test_check_incoming_continuation_condition(void){
  frame_header_t head;
  hstates_t st;
  head.stream_id = 440;
  head.length = 120;
  st.h2s.current_stream.stream_id = 440;
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.waiting_for_end_headers_flag = 1;
  uint32_t read_setting_from_returns[1] = {280};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "return code must be 0");
}

void test_check_incoming_continuation_condition_errors(void){
  frame_header_t head;
  hstates_t st;
  head.stream_id = 0;
  head.length = 120;
  st.h2s.current_stream.stream_id = 440;
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.waiting_for_end_headers_flag = 1;
  uint32_t read_setting_from_returns[1] = {280};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (incoming id equals 0)");
  head.stream_id = 442;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (stream mistmatch)");
  head.stream_id = 440;
  st.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (strean not open)");
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.state = STREAM_OPEN;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (not previous headers)");
  st.h2s.current_stream.state = STREAM_IDLE;
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (stream in idle)");
  head.length = 290;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (header length bigger than max)");

}

void test_handle_headers_payload_no_flags(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  int flag_returns[2] = {0, 0};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 1, "waiting end headers must be 1");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 1, "keep rcv must be 1");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 20, "pointer must be 20");
}

void test_handle_headers_payload_just_end_stream_flag(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  int flag_returns[2] = {1, 0};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 1, "waiting end headers must be 1");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 1, "keep rcv must be 1");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 20, "pointer must be 20");
  TEST_ASSERT_MESSAGE(st.h2s.received_end_stream == 1, "header ended strem flag was set");
}

void test_handle_headers_payload_full_message_header_no_end_stream(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  init_variables(&st);
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  // we receive 20 headers
  int rcv_returns[1] = {20};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  // only end_headers_flag is set
  int flag_returns[2] = {0, 1};
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waiting end headers must be 0. Full message received");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 0, "pointer must be 0");
  //TEST_ASSERT_MESSAGE(st.hd_lists.headers_in.count == 20, "count in must be 20");//needs mock to check this
  TEST_ASSERT_MESSAGE(st.new_headers == 1, "new headers received, so it must be 1");
}

void test_handle_headers_payload_full_message_header_end_stream(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  init_variables(&st);
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  // we receive 20 headers
  int rcv_returns[1] = {20};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  // both end_stream and end_header flags set
  int flag_returns[2] = {1, 1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waiting end headers must be 0. Full message received");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 0, "pointer must be 0");
  //TEST_ASSERT_MESSAGE(st.hd_lists.headers_in.count == 20, "count in must be 20");//needs mock to chek this
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_HALF_CLOSED_REMOTE, "Stream must be HALF CLOSED REMOTE");
  TEST_ASSERT_MESSAGE(st.h2s.received_end_stream == 0, "received end stream was reverted inside function, must be 0");
  TEST_ASSERT_MESSAGE(st.new_headers == 1, "new headers received, so it must be 1");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
}

void test_handle_headers_payload_errors(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  // First error, header block fragment too big
  uint32_t ghbfs_returns[2] = {10000, 20};
  SET_RETURN_SEQ(get_header_block_fragment_size, ghbfs_returns, 2);
  // Second error, buffer_copy invalid
  int bc_returns[2] = {-1, 20};
  SET_RETURN_SEQ(buffer_copy, bc_returns, 2);
  // Third error and fourth error, receive_header_block invalid
  int rcv_returns[3] = {-1, 25, 20};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 3);
  int flag_returns[6] = {0, 1, 0 ,1, 0, 1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 6);
  // Fifth error, header list size bigger than expected
  uint32_t headers_get_header_list_size_returns[1] = {129};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (Internal error)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (writting error)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (receive header error)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (hbf pointer invalid)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0 (http 431 error)");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receivig must be 0");
}

void test_handle_continuation_payload_no_end_headers_flag_set(void){
  frame_header_t head;
  continuation_payload_t cont;
  hstates_t st;
  int rc;
  // set payload size of 25 octets
  head.length = 20;
  // assume headers payload was received before, so block fragments were loaded too
  st.h2s.header_block_fragments_pointer = 50;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  int flag_returns[1] = {0};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 70, "pointer must be 75, new data was written");
}

void test_handle_continuation_payload_end_headers_flag_set(void){
  frame_header_t head;
  continuation_payload_t cont;
  hstates_t st;
  int rc;
  // set payload size of 25 octets
  head.length = 20;
  // assume headers payload was received before, so block fragments were loaded too
  st.h2s.header_block_fragments_pointer = 50;
  st.h2s.received_end_stream = 0;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rcv_returns[1] = {70};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 0, "pointer must be 0, fragments were received");
  //TEST_ASSERT_MESSAGE(st.hd_lists.headers_in.count == 70, "header list count in must be 70");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waiting for end headers must be 0");
  TEST_ASSERT_MESSAGE(st.new_headers == 1, "new headers received, so it must be 1");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
}

void test_handle_continuation_payload_end_headers_end_stream_flag_set(void){
  frame_header_t head;
  continuation_payload_t cont;
  hstates_t st;
  int rc;
  // set payload size of 25 octets
  head.length = 20;
  head.stream_id = 12;
  // assume headers payload was received before, so block fragments were loaded too
  st.h2s.header_block_fragments_pointer = 50;
  st.h2s.received_end_stream = 1;
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.current_stream.stream_id = 12;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  uint32_t headers_get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  int rcv_returns[1] = {70};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 0, "pointer must be 0, fragments were received");
  //TEST_ASSERT_MESSAGE(st.hd_lists.headers_in.count == 70, "header list count in must be 70");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waitinf for end headers must be 0");
  TEST_ASSERT_MESSAGE(st.new_headers == 1, "new headers received, so it must be 1");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_HALF_CLOSED_REMOTE, "stream must be half closed remote");
  TEST_ASSERT_MESSAGE(st.h2s.received_end_stream == 0, "received end stream must be 0");
}

void test_handle_continuation_payload_errors(void){
  frame_header_t head;
  continuation_payload_t cont;
  hstates_t st;
  int rc;
  // assume headers payload was received before, so block fragments were loaded too
  st.h2s.header_block_fragments_pointer = 50;
  st.h2s.received_end_stream = 0;
  // First error, header length too big
  head.length = 5000;
  // Second error, buffer copy invalid
  int bc_returns[2] = {-1, 20};
  SET_RETURN_SEQ(buffer_copy, bc_returns, 2);
  // Third and fourth error, receive_header_block invalid
  int rcv_returns[3] = {-1, 70, 110};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 3);
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  // Fourth error, header list size too big
  uint32_t headers_get_header_list_size_returns[1] = {129};
  SET_RETURN_SEQ(headers_get_header_list_size, headers_get_header_list_size_returns, 1);
  uint32_t read_setting_from_returns[1] = {128};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (length error)");
  head.length = 20;
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (buffer copy error)");
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (receive header block error)");
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (pointer error)");
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0 (http 431 error)");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
}

void test_send_headers_stream_verification_response_not_open(void){
  hstates_t hst;
  init_variables(&hst);
  int rc;
  rc = send_headers_stream_verification(&hst);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1, stream was not open");
}

void test_send_headers_stream_verification_response_not_init(void){
  hstates_t hst;
  init_variables(&hst);
  hst.h2s.current_stream.state = STREAM_OPEN;
  int rc;
  rc = send_headers_stream_verification(&hst);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1, stream was not initialized");
}

void test_send_headers_stream_verification_server(void){
  hstates_t hst_not_stream;
  memset(&hst_not_stream, 0, sizeof(hst_not_stream));
  hst_not_stream.is_server = 1;
  hst_not_stream.h2s.current_stream.stream_id = 2;
  hst_not_stream.h2s.current_stream.state = STREAM_IDLE;
  hstates_t hst_parity_stream;
  hst_parity_stream.is_server = 1;
  hst_parity_stream.h2s.last_open_stream_id = 431;
  hst_not_stream.h2s.current_stream.stream_id = 12;
  hst_parity_stream.h2s.current_stream.state = STREAM_IDLE;
  hstates_t hst_open_stream;
  hst_open_stream.is_server = 1;
  hst_open_stream.h2s.current_stream.stream_id = 5;
  hst_open_stream.h2s.current_stream.state = STREAM_OPEN;
  hstates_t hst_closed_stream;
  hst_closed_stream.is_server = 1;
  hst_closed_stream.h2s.current_stream.stream_id = 12;
  hst_closed_stream.h2s.current_stream.state = STREAM_CLOSED;
  int rc;
  rc = send_headers_stream_verification(&hst_not_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, stream was not initialized");
  TEST_ASSERT_MESSAGE(hst_not_stream.h2s.current_stream.stream_id == 2, "Error: new stream id must be 2");
  TEST_ASSERT_MESSAGE(hst_not_stream.h2s.current_stream.state == STREAM_OPEN, "Error: new stream state must be OPEN");
  rc = send_headers_stream_verification(&hst_parity_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, stream was not initialized");
  TEST_ASSERT_MESSAGE(hst_parity_stream.h2s.current_stream.state == STREAM_OPEN, "Error: new stream state must be OPEN");
  TEST_ASSERT_MESSAGE(hst_parity_stream.h2s.current_stream.stream_id == 432, "Error: new stream id must be 432");
  rc = send_headers_stream_verification(&hst_open_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, there was a previous stream open");
  TEST_ASSERT_MESSAGE(hst_open_stream.h2s.current_stream.stream_id == 5, "current stream id must remain equal");
  TEST_ASSERT_MESSAGE(hst_open_stream.h2s.current_stream.state == STREAM_OPEN, "stream state must remain equal");
  rc = send_headers_stream_verification(&hst_closed_stream);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1, there was a previous stream closed");
  TEST_ASSERT_MESSAGE(hst_closed_stream.h2s.current_stream.state == STREAM_CLOSED, "Stream state must be closed");
  TEST_ASSERT_MESSAGE(hst_closed_stream.h2s.current_stream.stream_id == 12, "Stream id must be 12");

}

void test_send_headers_stream_verification_client(void){
  hstates_t hst_not_stream;
  hst_not_stream.is_server = 0;
  hst_not_stream.h2s.current_stream.stream_id = 3;
  hst_not_stream.h2s.current_stream.state = STREAM_IDLE;
  hstates_t hst_parity_stream;
  hst_parity_stream.is_server = 0;
  hst_parity_stream.h2s.last_open_stream_id = 544;
  hst_parity_stream.h2s.current_stream.stream_id = 7;
  hst_parity_stream.h2s.current_stream.state = STREAM_IDLE;
  hstates_t hst_open_stream;
  hst_open_stream.is_server = 0;
  hst_open_stream.h2s.current_stream.stream_id = 5;
  hst_open_stream.h2s.current_stream.state = STREAM_OPEN;
  hstates_t hst_closed_stream;
  hst_closed_stream.is_server = 0;
  hst_closed_stream.h2s.current_stream.stream_id = 12;
  hst_closed_stream.h2s.current_stream.state = STREAM_CLOSED;
  int rc;
  rc = send_headers_stream_verification(&hst_not_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, stream was not initialized");
  TEST_ASSERT_MESSAGE(hst_not_stream.h2s.current_stream.stream_id == 3, "Error: new stream id must be 3");
  TEST_ASSERT_MESSAGE(hst_not_stream.h2s.current_stream.state == STREAM_OPEN, "Error: new stream state must be OPEN");
  rc = send_headers_stream_verification(&hst_parity_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, stream was not initialized");
  TEST_ASSERT_MESSAGE(hst_parity_stream.h2s.current_stream.state == STREAM_OPEN, "Error: new stream state must be OPEN");
  TEST_ASSERT_MESSAGE(hst_parity_stream.h2s.current_stream.stream_id == 545, "Error: new stream id must be 545");
  rc = send_headers_stream_verification(&hst_open_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, there was a previous stream open");
  TEST_ASSERT_MESSAGE(hst_open_stream.h2s.current_stream.stream_id == 5, "current stream id must remain equal");
  TEST_ASSERT_MESSAGE(hst_open_stream.h2s.current_stream.state == STREAM_OPEN, "stream state must remain equal");
  rc = send_headers_stream_verification(&hst_closed_stream);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1, there was a previous stream closed");
  TEST_ASSERT_MESSAGE(hst_closed_stream.h2s.current_stream.stream_id == 12, "current stream id must be 12");
  TEST_ASSERT_MESSAGE(hst_closed_stream.h2s.current_stream.state == STREAM_CLOSED, "stream state must be closed");
  rc = send_headers_stream_verification(&hst_not_stream);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0, stream was not initialized");
  TEST_ASSERT_MESSAGE(hst_not_stream.h2s.current_stream.stream_id == 3, "Error: stream id must be 3");
}

void test_send_headers_frame_all_branches(void){
  hstates_t st;
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  int create_headers_return[1] = {0};
  SET_RETURN_SEQ(create_headers_frame, create_headers_return, 1);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_headers_frame(&st, buff, 20, 0x16, 0, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0")
  rc = send_headers_frame(&st, buff, 20, 0x16, 1, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  rc = send_headers_frame(&st, buff, 20, 0x16, 1, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  rc = send_headers_frame(&st, buff, 20, 0x16, 0, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
}

void test_send_headers_frame(void){
  hstates_t st;
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  int create_headers_return[1] = {0};
  SET_RETURN_SEQ(create_headers_frame, create_headers_return, 1);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_headers_frame(&st, buff, 20, 0x16, 1, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "Create headers frame call count must be 1");
  TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 2, "Set flag call count must be 2");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes call count must be 1");
}

void test_send_headers_frame_errors(void){
  hstates_t st;
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  int create_headers_return[2] = {-1,0};
  SET_RETURN_SEQ(create_headers_frame, create_headers_return, 2);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_headers_frame(&st, buff, 20, 0x16, 1, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (create headers error)");
  size = HTTP2_MAX_BUFFER_SIZE;
  rc = send_headers_frame(&st, buff, 20, 0x16, 1, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (write error)");
}

void test_send_continuation_frame(void){
  hstates_t st;
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  int create_continuation_return[1] = {0};
  SET_RETURN_SEQ(create_continuation_frame, create_continuation_return, 1);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_continuation_frame(&st, buff, 20, 0x16, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(create_continuation_frame_fake.call_count == 1, "Create continuation frame call count must be 1");
  TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 1, "Set flag call count must be 1");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes call count must be 1");
}

void test_send_continuation_frame_errors(void){
  hstates_t st;
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  int create_continuation_return[2] = {-1,0};
  SET_RETURN_SEQ(create_continuation_frame, create_continuation_return, 2);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_continuation_frame(&st, buff, 20, 0x16, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (create continuation error)");
  size = HTTP2_MAX_BUFFER_SIZE;
  rc = send_continuation_frame(&st, buff, 20, 0x16,1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (write error)");
}

void test_send_headers_one_header(void){
  hstates_t st;
  st.is_server = 0;
  int rc = init_variables(&st);
  st.headers_out.count= 10;
  int create_headers_return[1] = {0};
  SET_RETURN_SEQ(create_headers_frame, create_headers_return, 1);
  int create_continuation_return[1] = {0};
  SET_RETURN_SEQ(create_continuation_frame, create_continuation_return, 1);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int compress_return[1] = {20};
  SET_RETURN_SEQ(compress_headers, compress_return, 1);
  uint32_t read_setting_from_return[1] = {20};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_return, 1);
  rc = send_headers(&st, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "Call count must be 1, one headers frame was created");
}

void test_send_headers_with_continuation(void){
  hstates_t st;
  st.is_server = 0;
  st.headers_in.count = 0;
  st.headers_out.count = 0;
  st.data_in.size = 0;
  st.data_out.size = 0;
  st.data_in.processed = 0;
  st.data_out.processed = 0;
  int rc = init_variables(&st);
  st.h2s.local_settings[4] = 20;
  st.headers_out.count = 10;
  int create_headers_return[1] = {0};
  SET_RETURN_SEQ(create_headers_frame, create_headers_return, 1);
  int create_continuation_return[1] = {0};
  SET_RETURN_SEQ(create_continuation_frame, create_continuation_return, 1);
  int frame_to_bytes_return[1] = {20};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);

  uint32_t read_setting_from_return[1] = {20};
  SET_RETURN_SEQ(read_setting_from, read_setting_from_return, 1);
  // Here fits 10 max sized frames (1 header, 9 cont) and one 1 byte frame (cont)
  int compress_return[1] = {201};
  SET_RETURN_SEQ(compress_headers, compress_return, 1);
  rc = send_headers(&st, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "Call count must be 1, one headers frame was created");
  TEST_ASSERT_MESSAGE(create_continuation_frame_fake.call_count == 10, "Call count must be 10, ten continuation frames were created");
}

void test_send_headers_errors(void){
  hstates_t st1; // First error, no headers to send
  st1.headers_out.count = 0;
  init_variables(&st1);
  hstates_t st2; // Second error, compress headers failure
  init_variables(&st2);
  int compress_return[2] = {-1, 20};
  st2.headers_out.count = 10;
  SET_RETURN_SEQ(compress_headers, compress_return, 2);
  uint32_t get_setting_return[1] = {20};
  SET_RETURN_SEQ(read_setting_from, get_setting_return, 1);
  hstates_t st3; // Third error, stream verification error
  init_variables(&st3);
  st3.headers_out.count = 10;
  st3.h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
  int rc = send_headers(&st1, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (no headers to send)");
  rc = send_headers(&st2, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (Compress headers error)");
  rc = send_headers(&st3, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream verification error)");
}

void test_handle_data_payload(void){

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    frame_header_t frame_header;
    data_payload_t data_payload;
    int data_size = 10;
    uint8_t data[data_size];
    uint8_t data_in_payload[]={1,2,3,4,5,6,7,8,9,0};
    uint32_t stream_id = 3;

    frame_header.type = DATA_TYPE;
    frame_header.flags = 0x0;
    frame_header.length = data_size;
    frame_header.stream_id = stream_id;
    frame_header.reserved = 0;
    buffer_copy(data, data_in_payload, data_size);
    data_payload.data = data;

    hstates_t st;
    st.headers_in.count = 0;
    st.headers_out.count = 0;
    st.data_in.size = 0;
    st.data_out.size = 0;
    st.data_in.processed = 0;
    st.data_out.processed = 0;
    int rc = init_variables(&st);
    rc = handle_data_payload(&frame_header, &data_payload, &st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(data_size,st.data_in.size);
    for(int i =0; i< data_size; i++){
        TEST_ASSERT_EQUAL(data_in_payload[i], st.data_in.buf[i]);
    }
}

int bytes_to_frame_header_fake_custom(uint8_t * bytes, int size, frame_header_t* frame_header){
    if(size!=9){
        return -1;
    }
    frame_header->length = bytes[2];
    frame_header->type = bytes[3];
    frame_header->flags = bytes[4];
    frame_header->stream_id = bytes[8];
    frame_header->reserved = 0;
    return 0;
}


int read_headers_payload_fake_custom(uint8_t* buff, frame_header_t* header, headers_payload_t* header_payload, uint8_t* headers_block_fragment, uint8_t* padding){
    buffer_copy(headers_block_fragment,buff,header->length);
    header_payload->header_block_fragment = headers_block_fragment;
    return header->length;
}

uint32_t get_header_block_fragment_size_fake_custom(frame_header_t* frame_header, headers_payload_t* header_payload){
    return frame_header->length;
}


int is_flag_set_fake_custom(uint8_t flags, uint8_t queried_flag){
    if((queried_flag&flags) >0){
        return 1;
    }
    return 0;
}

int receive_header_block_fake_custom(uint8_t* header_block, int size, headers_t* headers, hpack_dynamic_table_t* dynamic_table){
    strncpy(headers->headers[headers->count].name, (char*)"name", 4);
    strncpy(headers->headers[headers->count].value, (char*)"value",5);
    headers->count += 1;
    (void)dynamic_table;
    return size;
}

void test_h2_receive_frame_headers_wait_end_headers(void){
    hstates_t st;
    memset(&st, 0, sizeof(st));
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,10, 0x1, 0x0, 0,0,0,3,  0,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes,19);
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_headers_payload_fake.custom_fake = read_headers_payload_fake_custom;
    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    uint32_t read_setting_from_returns[1] = {128};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(1, st.h2s.waiting_for_end_headers_flag);
}


void test_h2_receive_frame_headers(void){
    hstates_t st;
    headers_t headers_in;
    headers_t headers_out;
    int maxlen = 2;
    header_t hlist_in[maxlen];
    header_t hlist_out[maxlen];
    headers_init_custom_fake(&headers_in, hlist_in, maxlen);
    headers_init_custom_fake(&headers_out, hlist_out, maxlen);
    st.headers_in = headers_in;
    st.headers_out = headers_out;

    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,10, 0x1, 0x4, 0,0,0,3,  0,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes,19);
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_headers_payload_fake.custom_fake = read_headers_payload_fake_custom;
    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    uint32_t read_setting_from_returns[1] = {128};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(0, st.h2s.waiting_for_end_headers_flag);
    TEST_ASSERT_EQUAL(1,st.headers_in.count);
    TEST_ASSERT_EQUAL(strncmp("name",st.headers_in.headers[0].name,4),0);
    TEST_ASSERT_EQUAL(strncmp("value",st.headers_in.headers[0].value,5),0);

}

void test_h2_receive_frame_data_stream_closed(void){
    hstates_t st;
    st.headers_in.count = 0;
    st.headers_out.count = 0;
    st.data_in.size = 0;
    st.data_out.size = 0;
    st.data_in.processed = 0;
    st.data_out.processed = 0;
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    uint8_t bytes[]={0,0,10, 0, 0, 0,0,0,3,   1,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes,19);
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    /*We write 200 zeros for future reading*/
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(-1,rc); //stream_closed_error  OK
}

int read_data_payload_fake_custom(uint8_t* buff_read,frame_header_t* frame_header, data_payload_t* data_payload, uint8_t* data) {
    buffer_copy(data, buff_read, frame_header->length);
    data_payload->data = data;
    return frame_header->length;
}

void test_h2_receive_frame_data_ok(void){
    hstates_t st;
    headers_t headers_in;
    headers_t headers_out;
    int maxlen = 2;
    header_t hlist_in[maxlen];
    header_t hlist_out[maxlen];
    headers_init_custom_fake(&headers_in, hlist_in, maxlen);
    headers_init_custom_fake(&headers_out, hlist_out, maxlen);
    st.headers_in = headers_in;
    st.headers_out = headers_out;
    st.data_in.size = 0;
    st.data_out.size = 0;
    st.data_in.processed = 0;
    st.data_out.processed = 0;
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,10, 0x1, 0x4, 0,0,0,3,  0,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes,19);

    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_headers_payload_fake.custom_fake = read_headers_payload_fake_custom;
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    uint32_t read_setting_from_returns[1] = {128};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);

    uint8_t bytes2[]={0,0,10, 0, 0, 0,0,0,3,   1,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes2,19);
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_data_payload_fake.custom_fake = read_data_payload_fake_custom;
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(10,st.data_in.size);
    for(int i = 0; i< 10;i++) {
        TEST_ASSERT_EQUAL(bytes2[i + 9], st.data_in.buf[i]);
    }

    TEST_ASSERT_EQUAL(10,flow_control_receive_data_fake.arg1_val);

}


int read_continuation_payload_fake_custom(uint8_t* buff_read, frame_header_t* frame_header, continuation_payload_t* continuation_payload, uint8_t * continuation_block_fragment){
    int rc = buffer_copy(continuation_block_fragment, buff_read, frame_header->length);
    continuation_payload->header_block_fragment = continuation_block_fragment;
    return rc;
}


void test_h2_receive_frame_continuation(void){
    hstates_t st;
    headers_t headers_in;
    headers_t headers_out;
    int maxlen = 2;
    header_t hlist_in[maxlen];
    header_t hlist_out[maxlen];
    headers_init_custom_fake(&headers_in, hlist_in, maxlen);
    headers_init_custom_fake(&headers_out, hlist_out, maxlen);
    st.headers_in = headers_in;
    st.headers_out = headers_out;
    st.data_in.size = 0;
    st.data_out.size = 0;
    st.data_in.processed = 0;
    st.data_out.processed = 0;
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,10, 0x1, 0x0, 0,0,0,3,  1,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes,19);

    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_headers_payload_fake.custom_fake = read_headers_payload_fake_custom;
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    uint32_t read_setting_from_returns[1] = {128};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(1, st.h2s.waiting_for_end_headers_flag);
    TEST_ASSERT_EQUAL(10, st.h2s.header_block_fragments_pointer);
    for(int i =0; i< 10; i++) {
        TEST_ASSERT_EQUAL(bytes[i+9], st.h2s.header_block_fragments[i]);
    }
    uint8_t bytes2[]={0,0,10, 0x9, 0x4, 0,0,0,3,   1,2,3,4,5,6,7,8,9,0};
    http_write(&st,bytes2,19);
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_data_payload_fake.custom_fake = read_data_payload_fake_custom;
    read_continuation_payload_fake.custom_fake = read_continuation_payload_fake_custom;
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(0, st.h2s.waiting_for_end_headers_flag);
    //TEST_ASSERT_EQUAL(20, st.h2s.header_block_fragments_pointer);
    //for(int i =0; i< 10; i++) {
    //    TEST_ASSERT_EQUAL(bytes[i+19], st.h2s.header_block_fragments[i]);
    //}

    TEST_ASSERT_EQUAL(1,st.headers_in.count);
    TEST_ASSERT_EQUAL(strncmp("name",st.headers_in.headers[0].name,4),0);
    TEST_ASSERT_EQUAL(strncmp("value",st.headers_in.headers[0].value,5),0);
}


int read_window_update_payload_fake_custom(uint8_t* buff_read, frame_header_t* frame_header, window_update_payload_t* window_update_payload){
    if(frame_header->length!=4){
        return -1;
    }
    uint32_t window_size_increment = (uint32_t)buff_read[3];
    window_update_payload->window_size_increment = window_size_increment;
    return frame_header->length;
}


void test_h2_receive_frame_window_update(void){
    hstates_t st;

    int rc = init_variables(&st);
    st.is_server = 1;
    st.h2s.outgoing_window.window_used = 30;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,4, 0x8, 0x0, 0,0,0,3,  0,0,0,10};//window_update frame->window_size_increment = 10
    http_write(&st,bytes,13);
    uint32_t read_setting_from_returns[1] = {1024};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    read_window_update_payload_fake.custom_fake = read_window_update_payload_fake_custom;
    //is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(10,flow_control_receive_window_update_fake.arg1_val);
}



int bytes_to_settings_payload_fake_custom(uint8_t*bytes, int size, settings_payload_t*settings_payload, settings_pair_t*pairs){
    if(size%6!=0){
        return -1;
    }
    pairs[0].identifier = bytes[1];
    pairs[0].value = bytes[5];
    settings_payload->pairs = pairs;
    settings_payload->count = 1;
    return 6*1;
}

void test_h2_receive_frame_settings(void){
    hstates_t st;
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,6, 0x4, 0x0, 0,0,0,0,  0,1, 0,0,0,10};//settings frame no ACK setting 1 = 10
    http_write(&st,bytes,15);

    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    //read_settings_payload_fake.custom_fake = read_settings_payload_fake_custom;
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    bytes_to_settings_payload_fake.custom_fake = bytes_to_settings_payload_fake_custom;
    uint32_t read_setting_from_returns[1] = {128};
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(10,st.h2s.remote_settings[0]);

}


void test_h2_receive_frame_settings_ack(void){
    hstates_t st;
    st.h2s.wait_setting_ack = 1;
    int rc = init_variables(&st);
    st.is_server = 1;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    get_header_block_fragment_size_fake.custom_fake = get_header_block_fragment_size_fake_custom;
    uint8_t bytes[]={0,0,0, 0x4, 0x1, 0,0,0,0};//settings frame no ACK setting 1 = 10
    http_write(&st,bytes,9);

    receive_header_block_fake.custom_fake = receive_header_block_fake_custom;
    bytes_to_frame_header_fake.custom_fake = bytes_to_frame_header_fake_custom;
    //read_settings_payload_fake.custom_fake = read_settings_payload_fake_custom;
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    bytes_to_settings_payload_fake.custom_fake = bytes_to_settings_payload_fake_custom;
    rc = h2_receive_frame(&st);
    TEST_ASSERT_EQUAL(0,rc);
    TEST_ASSERT_EQUAL(0,st.h2s.wait_setting_ack);
}


void test_send_data(void){
  hstates_t st;
  st.headers_in.count = 0;
  st.headers_out.count = 0;
  st.data_in.size = 0;
  st.data_out.size = 0;
  st.data_in.processed = 0;
  st.data_out.processed = 0;
  int rc = init_variables(&st);
  st.data_out.size = 36;
  st.h2s.outgoing_window.window_size = 30;
  st.h2s.current_stream.state = STREAM_OPEN;
  uint32_t get_return[1] = {30};
  SET_RETURN_SEQ(get_size_data_to_send, get_return, 1);
  rc = send_data(&st, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.data_out.processed == 30, "Data out sent must be 30, full data was sent");
  TEST_ASSERT_MESSAGE(st.data_out.size == 36, "Data out size must be 0, full data was sent");
}

void test_send_data_full_sending(void){
  hstates_t st;
  st.headers_in.count = 0;
  st.headers_out.count = 0;
  st.data_in.size = 0;
  st.data_out.size = 0;
  st.data_in.processed = 0;
  st.data_out.processed = 0;
  int rc = init_variables(&st);
  st.data_out.size = 27;
  st.h2s.outgoing_window.window_size = 50;
  st.h2s.current_stream.state = STREAM_OPEN;
  uint32_t get_return[1] = {27};
  SET_RETURN_SEQ(get_size_data_to_send, get_return, 1);
  rc = send_data(&st, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.data_out.processed == 0, "Data out sent must be 0, full data was sent");
  TEST_ASSERT_MESSAGE(st.data_out.size == 0, "Data out size must be 0, full data was sent");
}


void test_send_window_update(void){
    hstates_t st;
    int rc = init_variables(&st);
    st.h2s.incoming_window.window_used = 30;
    st.h2s.current_stream.state = STREAM_OPEN;
    uint8_t window_size_increment = 10;
    rc = send_window_update(&st, window_size_increment);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0 (there were no problems to send window update)");
    TEST_ASSERT_EQUAL(10, flow_control_send_window_update_fake.arg1_val);
}

void test_send_data_errors(void){
  int data_create_return[2] = {-1,0};
  SET_RETURN_SEQ(create_data_frame, data_create_return, 2);
  int frame_bytes_ret[2] = {10,10};
  SET_RETURN_SEQ(frame_to_bytes, frame_bytes_ret, 2);
  hstates_t st1; //First error, no data to sned
    st1.headers_in.count = 0;
    st1.headers_out.count = 0;
    st1.data_in.size = 0;
    st1.data_out.size = 0;
    st1.data_in.processed = 0;
    st1.data_out.processed = 0;
  int rc = init_variables(&st1);
  st1.data_out.size = 0;
  rc = send_data(&st1, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (no data out)");
  hstates_t st2; //Second error, wrong state stream
    st2.headers_in.count = 0;
    st2.headers_out.count = 0;
    st2.data_in.size = 0;
    st2.data_out.size = 0;
    st2.data_in.processed = 0;
    st2.data_out.processed = 0;
  rc = init_variables(&st2);
  st2.data_out.size = 27;
  st2.data_out.processed = 0;
  st2.h2s.outgoing_window.window_size = 50;
  st2.h2s.outgoing_window.window_used = 0;
  st2.h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
  rc = send_data(&st2, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream half closed local)");
  hstates_t st3; //Third error, create_data_frame error
    st3.headers_in.count = 0;
    st3.headers_out.count = 0;
    st3.data_in.size = 0;
    st3.data_out.size = 0;
    st3.data_in.processed = 0;
    st3.data_out.processed = 0;
  rc = init_variables(&st3);
  st3.data_out.size = 27;
  st3.data_out.processed = 0;
  st3.h2s.outgoing_window.window_size = 50;
  st3.h2s.outgoing_window.window_used = 0;
  st3.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
  rc = send_data(&st3, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (create_data_frame error)");
  hstates_t st4; // Fourth error, writting error
  rc = init_variables(&st4);
  st4.data_out.size = 27;
  st4.data_out.processed = 0;
  st4.h2s.outgoing_window.window_size = 50;
  st4.h2s.outgoing_window.window_used = 0;
  st4.h2s.current_stream.state = STREAM_OPEN;
  size = HTTP2_MAX_BUFFER_SIZE;
  rc = send_data(&st4, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (writting error)");
}

void test_send_goaway(void){
    hstates_t st;
    int rc = init_variables(&st);
    rc = send_goaway(&st, HTTP2_NO_ERROR);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    int frame_to_bytes_return[1] = {200};
    SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
    rc = send_goaway(&st, HTTP2_NO_ERROR);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
}

void test_send_goaway_errors(void){
  hstates_t st;
  int rc = init_variables(&st);
  int create_goaway_return[2] = {-1, 0};
  SET_RETURN_SEQ(create_goaway_frame, create_goaway_return, 2);
  rc = send_goaway(&st, HTTP2_NO_ERROR);
  TEST_ASSERT_MESSAGE(rc < 0, "Return code must be less than 0");
  int frame_to_bytes_return[1] = {30000};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  rc = send_goaway(&st, HTTP2_NO_ERROR);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (write error)" );
}
// TODO test_send_goaway_errors

void test_change_stream_state_end_stream_flag(void){
    hstates_t st;
    st.h2s.current_stream.state = STREAM_OPEN;
    int rc = change_stream_state_end_stream_flag(&st, 0); // receving flag
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_HALF_CLOSED_REMOTE, "Stream state must be STREAM_HALF_CLOSED_REMOTE");
    st.h2s.current_stream.state = STREAM_OPEN;
    rc = change_stream_state_end_stream_flag(&st, 1); // sending
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "Stream state must be STREAM_HALF_CLOSED_LOCAL");

    st.h2s.received_goaway = 0;
    st.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    rc = change_stream_state_end_stream_flag(&st, 1); // sending
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->h2s.current_stream.state);
    st.h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    rc = change_stream_state_end_stream_flag(&st, 0); // receving
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->h2s.current_stream.state);

    // TODO branches with received_goaway = 1
    st.h2s.received_goaway = 1;
    st.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    rc = change_stream_state_end_stream_flag(&st, 1); // sending
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_CLOSED, "Stream state must be STREAM_CLOSED");

    st.h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    rc = change_stream_state_end_stream_flag(&st, 0); // receiving
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_CLOSED, "Stream state must be STREAM_CLOSED");

}

void test_change_stream_state_end_stream_flag_errors(void){
  hstates_t st;
  st.h2s.received_goaway = 1;
  int create_goaway_return[1] = {1};
  SET_RETURN_SEQ(create_goaway_frame, create_goaway_return, 1);
  size = HTTP2_MAX_BUFFER_SIZE + 1;
  st.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
  int rc = change_stream_state_end_stream_flag(&st, 1); // sending
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_CLOSED, "Stream state must be STREAM_HALF_CLOSED_REMOTE");

  st.h2s.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
  rc = change_stream_state_end_stream_flag(&st, 0); // receiving
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_CLOSED, "Stream state must be STREAM_HALF_CLOSED_LOCAL");

}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_init_variables);
    UNIT_TEST(test_update_settings_table);
    UNIT_TEST(test_update_settings_table_errors);
    UNIT_TEST(test_send_settings_ack);
    UNIT_TEST(test_send_settings_ack_errors);
    UNIT_TEST(test_check_incoming_settings_condition);
    UNIT_TEST(test_check_incoming_settings_condition_errors);
    UNIT_TEST(test_handle_settings_payload);
    UNIT_TEST(test_handle_settings_payload_errors);
    UNIT_TEST(test_read_frame);
    UNIT_TEST(test_read_frame_errors);
    UNIT_TEST(test_send_local_settings);
    UNIT_TEST(test_send_local_settings_errors);
    UNIT_TEST(test_h2_client_init_connection);
    UNIT_TEST(test_h2_client_init_connection_errors);
    UNIT_TEST(test_h2_server_init_connection);
    UNIT_TEST(test_h2_server_init_connection_errors);
    UNIT_TEST(test_check_incoming_headers_condition);
    UNIT_TEST(test_check_incoming_headers_condition_error);
    //UNIT_TEST(test_check_incoming_headers_condition_creation_of_stream);
    UNIT_TEST(test_check_incoming_headers_condition_mismatch);
    UNIT_TEST(test_check_incoming_continuation_condition);
    UNIT_TEST(test_check_incoming_continuation_condition_errors);
    UNIT_TEST(test_handle_headers_payload_no_flags);
    UNIT_TEST(test_handle_headers_payload_just_end_stream_flag);
    UNIT_TEST(test_handle_headers_payload_full_message_header_no_end_stream);
    UNIT_TEST(test_handle_headers_payload_full_message_header_end_stream);
    //UNIT_TEST(test_handle_headers_payload_errors);
    UNIT_TEST(test_handle_continuation_payload_no_end_headers_flag_set);
    UNIT_TEST(test_handle_continuation_payload_end_headers_flag_set);
    UNIT_TEST(test_handle_continuation_payload_end_headers_end_stream_flag_set);
    //UNIT_TEST(test_handle_continuation_payload_errors);
    UNIT_TEST(test_send_headers_stream_verification_server);
    UNIT_TEST(test_send_headers_stream_verification_client);
    UNIT_TEST(test_send_headers_frame);
    UNIT_TEST(test_send_headers_frame_all_branches);
    UNIT_TEST(test_send_headers_frame_errors);
    UNIT_TEST(test_send_continuation_frame);
    UNIT_TEST(test_send_continuation_frame_errors);
    UNIT_TEST(test_send_headers_one_header);
    UNIT_TEST(test_send_headers_with_continuation);
    UNIT_TEST(test_send_headers_errors);



    UNIT_TEST(test_handle_data_payload);
    UNIT_TEST(test_h2_receive_frame_headers);
    UNIT_TEST(test_h2_receive_frame_headers_wait_end_headers);

    UNIT_TEST(test_send_data);
    UNIT_TEST(test_send_data_full_sending);
    UNIT_TEST(test_send_data_errors);

    UNIT_TEST(test_h2_receive_frame_data_stream_closed);
    UNIT_TEST(test_h2_receive_frame_data_ok);
    UNIT_TEST(test_h2_receive_frame_continuation);
    UNIT_TEST(test_h2_receive_frame_window_update);
    UNIT_TEST(test_h2_receive_frame_settings);
    UNIT_TEST(test_h2_receive_frame_settings_ack);



    UNIT_TEST(test_send_window_update);

    UNIT_TEST(test_send_goaway);
    UNIT_TEST(test_send_goaway_errors);
    UNIT_TEST(test_change_stream_state_end_stream_flag);
    UNIT_TEST(test_change_stream_state_end_stream_flag_errors);

    //TODO:
    // h2_receive_frame
    // send_data
    // h2_send_request
    // h2_send_response


    return UNIT_TESTS_END();
}
