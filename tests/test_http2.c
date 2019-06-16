#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2.h"
#include "http2utils.h"
 /*---------- Import of functions not declared in http2.h ----------------*/
extern int init_variables(hstates_t * st);
extern int update_settings_table(settings_payload_t *spl, uint8_t place, hstates_t *st);
extern int send_settings_ack(hstates_t *st);
extern int check_for_settings_ack(frame_header_t *header, hstates_t *st);
extern int handle_settings_payload(uint8_t *bf, frame_header_t *h, settings_payload_t *spl, settings_pair_t *pairs, hstates_t *st);
extern int read_frame(uint8_t *buff_read, frame_header_t *header);
extern int check_incoming_headers_condition(frame_header_t *header, hstates_t *st);
extern int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, hstates_t *st);
extern int check_incoming_continuation_condition(frame_header_t *header, hstates_t *st);
extern int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, hstates_t *st);
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
     puts("Error in read function");
     return -1;
   }
   read_bytes = read_bytes + incoming_bytes;
 }
 return read_bytes;
}

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, verify_setting, uint16_t, uint32_t);
FAKE_VALUE_FUNC(int, create_settings_ack_frame, frame_t *, frame_header_t*);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t*, uint8_t*);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, bytes_to_settings_payload, uint8_t*, int, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, bytes_to_frame_header, uint8_t*, int , frame_header_t*);
FAKE_VALUE_FUNC(int, create_settings_frame ,uint16_t*, uint32_t*, int, frame_t*, frame_header_t*, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t*, uint8_t*, int);
FAKE_VALUE_FUNC(int, get_header_block_fragment_size,frame_header_t*, headers_payload_t*);
FAKE_VALUE_FUNC(int, receive_header_block,uint8_t*, int, table_pair_t*, uint8_t);
FAKE_VALUE_FUNC(int, read_headers_payload, uint8_t*, frame_header_t*, headers_payload_t*, uint8_t*, uint8_t*);//TODO fix this
FAKE_VALUE_FUNC(int, read_continuation_payload, uint8_t*, frame_header_t*, continuation_payload_t*, uint8_t*);//TODO fix this
FAKE_VALUE_FUNC(uint32_t, get_setting_value, uint32_t*, sett_param_t)
FAKE_VALUE_FUNC(uint32_t, get_header_list_size, table_pair_t*, uint8_t)

#define FFF_FAKES_LIST(FAKE)              \
    FAKE(verify_setting)                  \
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
    FAKE(get_header_list_size)            \
/*----------Value Return for FAKEs ----------*/
int verify_return_zero(uint16_t u, uint32_t uu){
  return 0;
}
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
int ghbfs(frame_header_t *h, headers_payload_t *st){
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
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
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
  TEST_ASSERT_MESSAGE(hdummy.h2s.current_stream.stream_id == 0, "Current stream id must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.current_stream.state == 0, "Current stream state must be 0");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
}

void test_update_settings_table(void){
  settings_pair_t pair1 = {0x1, 12345};
  settings_pair_t pair2 = {0x2, 0};
  settings_pair_t pair3 = {0x3, 12345};
  settings_pair_t pair4 = {0x4, 12345};
  settings_pair_t pair5 = {0x5, 12345};
  settings_pair_t pair6 = {0x6, 12345};
  settings_pair_t pairs[6] = {pair1,pair2,pair3,pair4,pair5,pair6};
  settings_payload_t payload = {pairs, 6};
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  hdummy.h2s = dummy;
  verify_setting_fake.custom_fake = verify_return_zero;
  int rc = update_settings_table(&payload, LOCAL, &hdummy);
  TEST_ASSERT_MESSAGE(verify_setting_fake.call_count == 6, "Call count of verify_setting must be 6");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[0] == 12345, "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[1] == 0, "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[2] == 12345, "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[3] == 12345, "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[4] == 12345, "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[5] == 12345, "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[0] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[1] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  rc = update_settings_table(&payload, REMOTE, &hdummy);
  TEST_ASSERT_MESSAGE(verify_setting_fake.call_count == 12, "Call count of verify_setting must be 12");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[0] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[1] == 0, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[2] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[3] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[4] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.local_settings[5] == 12345, "Local settings were not modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[0] == 12345, "HTS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[1] == 0, "EP in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 12345, "MCS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 12345, "IWS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 12345, "MFS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[5] == 12345, "MHLS in remote settings must be modified!");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");

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
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  hdummy.h2s = dummy;
  int verify_return[12];
  memset(verify_return, 0, sizeof(verify_return));
  // First error, one verification fails
  verify_return[3] = -1;
  SET_RETURN_SEQ(verify_setting, verify_return, 12);
  int rc = update_settings_table(&payload, LOCAL, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1, invalid setting inserted");
  rc = update_settings_table(&payload, 5, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1, not valid table");
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
  int frame_to_bytes_return[1] = {1000};
  SET_RETURN_SEQ(frame_to_bytes, frame_to_bytes_return, 1);
  int rc = send_settings_ack(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in creation)");
  TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count = 1, "ACK creation not called");
  rc = send_settings_ack(&hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in settings write)");
}

void test_check_for_settings_ack(void){
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      1};
  hdummy.h2s = dummy;
  frame_header_t header_ack = {0, 0x4, 0x0|0x1, 0x0, 0};
  frame_header_t header_not_ack = {24, 0x4, 0x0, 0x0, 0};
  int flag_returns[3] = {0, 1, 1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 3);
  int rc = check_for_settings_ack(&header_not_ack, &hdummy);
  TEST_ASSERT_MESSAGE(is_flag_set_fake.call_count == 1, "is flag set must be called once");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0. ACK flag is not setted");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 1, "wait must remain in 1");
  rc = check_for_settings_ack(&header_ack, &hdummy);
  TEST_ASSERT_MESSAGE(is_flag_set_fake.call_count == 2, "is flag set must be called for second time");
  TEST_ASSERT_MESSAGE(rc == 1, "RC must be 1. ACK flag was setted and payload size was 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 0, "wait must be changed to 0");
  rc = check_for_settings_ack(&header_ack, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 1, "RC must be 1. ACK flag was setted, but not wait ack flag asigned");
}

void test_check_for_settings_ack_errors(void){
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      1};
  hdummy.h2s = dummy;
  // first error, wrong type
  frame_header_t header_ack_wrong_type = {0, 0x5, 0x0|0x1, 0x0, 0};
  // second error, wrong stream_id
  frame_header_t header_ack_wrong_stream = {0, 0x4, 0x0|0x1, 0x1, 0};
  header_ack_wrong_stream.stream_id = 1;
  // third error, wrong size
  frame_header_t header_ack_wrong_size = {24, 0x4, 0x0|0x1, 0x0, 0};
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  int rc = check_for_settings_ack(&header_ack_wrong_type, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (wrong type)");
  rc = check_for_settings_ack(&header_ack_wrong_stream, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (wrong stream)");
  rc = check_for_settings_ack(&header_ack_wrong_size, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (wrong size)");
}

void test_handle_settings_payload(void){
  settings_pair_t pair1 = {0x3, 12345};
  settings_pair_t pair2 = {0x4, 12345};
  settings_pair_t pair3 = {0x5, 12345};
  settings_pair_t pair4 = {0x6, 12345};
  settings_pair_t pairs[4] = {pair1,pair2,pair3,pair4};
  settings_payload_t payload = {pairs, 4};
  frame_header_t header_sett = {24, 0x4, 0x0|0x1, 0x0, 0};
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  hdummy.h2s = dummy;
  bytes_to_settings_payload_fake.custom_fake = bytes_settings_payload_return_24;
  create_settings_ack_frame_fake.custom_fake = create_ack_return_zero;
  verify_setting_fake.custom_fake = verify_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_9;
  int rc = handle_settings_payload(buffer, &header_sett, &payload, pairs, &hdummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 0, "ACK wait must be 0");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[2] == 12345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[3] == 12345, "Remote settings were not updates!");
  TEST_ASSERT_MESSAGE(hdummy.h2s.remote_settings[4] == 12345, "Remote settings were not updates!");
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
  frame_header_t header_sett = {24, 0x4, 0x0|0x1, 0x0, 0};
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  hdummy.h2s = dummy;
  // First error, bytes to settings payload fail
  int bytes_return[2] = {-1, 24};
  SET_RETURN_SEQ(bytes_to_settings_payload, bytes_return, 2);
  create_settings_ack_frame_fake.custom_fake = create_ack_return_zero;
  int verify_return[1] = {-1};
  SET_RETURN_SEQ(verify_setting, verify_return, 2);
  frame_to_bytes_fake.custom_fake = frame_bytes_return_9;
  int rc = handle_settings_payload(buffer, &header_sett, &payload, pairs, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in bytes to settings payload)");
  rc = handle_settings_payload(buffer, &header_sett, &payload, pairs, &hdummy);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (error in update settings table)");
}

void test_read_frame(void){
  hstates_t hst;
  frame_header_t header = {36, 0x4, 0x0, 0x0, 0};
  uint8_t bf[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  bytes_to_frame_header_fake.custom_fake = bytes_frame_return_zero;
  /*We write 200 zeros for future reading*/
  int wrc = http_write(&hst, bf, 200);
  TEST_ASSERT_MESSAGE(wrc == 200, "wrc must be 200.");
  TEST_ASSERT_MESSAGE(size == 200, "size must be 200.");
  int rc = read_frame(bf, &header);
  TEST_ASSERT_MESSAGE(bytes_to_frame_header_fake.call_count == 1, "bytes to frame header must be called once");
  TEST_ASSERT_MESSAGE(size == 155, "read_frame must have read 45 bytes");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
}

void test_read_frame_error(void){
  hstates_t hst;
  /*Header with payload greater than 256*/
  frame_header_t bad_header = {1024, 0x4, 0x0, 0x0, 0};
  frame_header_t good_header = {200, 0x4, 0x0, 0x0, 0};
  uint8_t bf[HTTP2_MAX_BUFFER_SIZE] = { 0 };
  // Second error, bytes_to_frame_header return
  int bytes_return[2] = {-1, 0};
  SET_RETURN_SEQ(bytes_to_frame_header, bytes_return, 2);
  // First error, there is no data to read
  int rc = read_frame(bf, &good_header);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (read_n_bytes error)");
  int wrc = http_write(&hst, bf, 200);
  TEST_ASSERT_MESSAGE(wrc == 200, "wrc must be 200.");
  TEST_ASSERT_MESSAGE(size == 200, "size must be 200.");
  // Second error
  rc = read_frame(bf, &good_header);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (bytes_to_frame_header error)");
  /*We write 200 zeros for future reading*/
  // Third error, payload size too big
  rc = read_frame(bf, &bad_header);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (payload size error)");
  TEST_ASSERT_MESSAGE(bytes_to_frame_header_fake.call_count == 2, "bytes to frame header must be called once");
  TEST_ASSERT_MESSAGE(size == 182, "read_frame must have read  bytes of header");
  // Fourth error, read_n_bytes payload reading error
  rc = read_frame(bf, &good_header);
  TEST_ASSERT_MESSAGE(rc == -1, "RC must be -1 (payload read error)");
}

void test_h2_send_local_settings(void){
  /*Depends on create_settings_frame, frame_to_bytes and http_write*/
  hstates_t hdummy;
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  hdummy.h2s = dummy;
  create_settings_frame_fake.custom_fake = create_return_zero;
  frame_to_bytes_fake.custom_fake = frame_bytes_return_45;
  int rc = h2_send_local_settings(&hdummy);
  TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame_to_bytes must be called once");
  TEST_ASSERT_MESSAGE(size == 45, "send local settings must have written 45 bytes");
  TEST_ASSERT_MESSAGE(hdummy.h2s.wait_setting_ack == 1, "wait_setting_ack must be 1. Settings were sent");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of h2_send_local_settings must be 0");
}

void test_h2_read_setting_from(void){
  hstates_t hdummy;
  h2states_t dummy = {{1,2,3,4,5,6},
                      {7,8,9,10,11,12},
                      0};
  hdummy.h2s = dummy;
  uint32_t answ = h2_read_setting_from(LOCAL, 0x1, &hdummy);
  TEST_ASSERT_MESSAGE(answ == 7, "Answer must be 7");
  answ = h2_read_setting_from(REMOTE, 0x6, &hdummy);
  TEST_ASSERT_MESSAGE(answ == 6, "Answer must be 6");
  answ = h2_read_setting_from(LOCAL, 0x0, &hdummy);
  TEST_ASSERT_MESSAGE(answ == -1, "Answer must be -1. Error in id! (overvalue)");
  answ = h2_read_setting_from(REMOTE, 0x7, &hdummy);
  TEST_ASSERT_MESSAGE(answ == -1, "Answer mus be -1. Error in id! (uppervalue)");
}

void test_h2_client_init_connection(void){
  /*Depends on http_write and h2_send_local_settings*/
  hstates_t client;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
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

void test_h2_server_init_connection(void){
  /*Depends on http_write and h2_send_local_settings*/
  hstates_t server;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
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

void test_check_incoming_headers_condition(void){
  frame_header_t head;
  head.stream_id = 2440;
  hstates_t st;
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.stream_id = 2440;
  st.h2s.current_stream.state = STREAM_OPEN;
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

}

void test_check_incoming_headers_condition_error(void){
  frame_header_t head;
  hstates_t st;
  st.h2s.waiting_for_end_headers_flag = 1;
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1");
}

void test_check_incoming_headers_condition_creation_of_stream(void){
  frame_header_t head;
  head.stream_id = 2440;
  hstates_t st;
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.stream_id = 0;
  st.h2s.current_stream.state = 0;
  int rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
  st.h2s.current_stream.stream_id = 2438;
  st.h2s.current_stream.state = STREAM_CLOSED;
  rc = check_incoming_headers_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
  TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

}

void test_check_incoming_headers_condition_mismatch(void){
  frame_header_t head;
  head.stream_id = 2440;
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
  TEST_ASSERT_MESSAGE(rc == -2, "Return code must be -2");
}

void test_check_incoming_continuation_condition(void){
  frame_header_t head;
  hstates_t st;
  head.stream_id = 440;
  st.h2s.current_stream.stream_id = 440;
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.waiting_for_end_headers_flag = 1;
  int rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "return code must be 0");
}

void test_check_incoming_continuation_condition_errors(void){
  frame_header_t head;
  hstates_t st;
  head.stream_id = 0;
  st.h2s.current_stream.stream_id = 440;
  st.h2s.current_stream.state = STREAM_OPEN;
  st.h2s.waiting_for_end_headers_flag = 1;
  int rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (incoming id equals 0)");
  head.stream_id = 442;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (stream mistmatch)");
  head.stream_id = 440;
  st.h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -2, "return code must be -1 (strean not open)");
  st.h2s.waiting_for_end_headers_flag = 0;
  st.h2s.current_stream.state = STREAM_OPEN;
  rc = check_incoming_continuation_condition(&head, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "return code must be -1 (not previous headers)");
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
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
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
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
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
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  // we receive 25 headers
  int rcv_returns[3] = {25};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  // only end_headers_flag is set
  int flag_returns[2] = {0, 1};
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waiting end headers must be 0. Full message received");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 20, "pointer must be 20");
  TEST_ASSERT_MESSAGE(st.h_lists.header_list_count_in == 25, "count in must be 25");
  TEST_ASSERT_MESSAGE(st.new_headers == 1, "new headers received, so it must be 1");
}

void test_handle_headers_payload_full_message_header_end_stream(void){
  frame_header_t head;
  headers_payload_t hpl;
  hstates_t st;
  st.h2s.header_block_fragments_pointer = 123;
  // returns 20
  get_header_block_fragment_size_fake.custom_fake = ghbfs;
  // returns 20
  buffer_copy_fake.custom_fake = bc;
  // we receive 25 headers
  int rcv_returns[3] = {25};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  // both end_stream and end_header flags set
  int flag_returns[2] = {1, 1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.waiting_for_end_headers_flag == 0, "waiting end headers must be 0. Full message received");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 20, "pointer must be 20");
  TEST_ASSERT_MESSAGE(st.h_lists.header_list_count_in == 25, "count in must be 25");
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
  int ghbfs_returns[2] = {10000, 20};
  SET_RETURN_SEQ(get_header_block_fragment_size, ghbfs_returns, 2);
  // Second error, buffer_copy invalid
  int bc_returns[2] = {-1, 20};
  SET_RETURN_SEQ(buffer_copy, bc_returns, 2);
  // Third error, receive_header_block invalid
  int rcv_returns[2] = {0, 25};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 2);
  int flag_returns[4] = {0, 1, 0 ,1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 4);
  // Fourth error, header list size bigger than expected
  uint32_t get_header_list_size_returns[1] = {129};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (Internal error)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (writting error)");
  rc = handle_headers_payload(&head, &hpl, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (receive header error)");
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
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
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
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rcv_returns[1] = {70};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 70, "pointer must be 75, new data was written");
  TEST_ASSERT_MESSAGE(st.h_lists.header_list_count_in == 70, "header list count in must be 70");
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
  uint32_t get_header_list_size_returns[1] = {120};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  int rcv_returns[1] = {70};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(st.h2s.header_block_fragments_pointer == 70, "pointer must be 75, new data was written");
  TEST_ASSERT_MESSAGE(st.h_lists.header_list_count_in == 70, "header list count in must be 70");
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
  head.length = 300;
  // Second error, buffer copy invalid
  int bc_returns[2] = {-1, 20};
  SET_RETURN_SEQ(buffer_copy, bc_returns, 2);
  // Third error, receive_header_block invalid
  int rcv_returns[2] = {0, 25};
  SET_RETURN_SEQ(receive_header_block, rcv_returns, 2);
  int flag_returns[1] = {1};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
  // Fourth error, header list size too big
  uint32_t get_header_list_size_returns[1] = {129};
  SET_RETURN_SEQ(get_header_list_size, get_header_list_size_returns, 1);
  uint32_t get_setting_value_returns[1] = {128};
  SET_RETURN_SEQ(get_setting_value, get_setting_value_returns, 1);
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (length error)");
  head.length = 20;
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (buffer copy error)");
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (receive header block error)");
  rc = handle_continuation_payload(&head, &cont, &st);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be -1 (http 431 error)");
  TEST_ASSERT_MESSAGE(st.keep_receiving == 0, "keep receiving must be 0");
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_init_variables);
    UNIT_TEST(test_update_settings_table);
    UNIT_TEST(test_update_settings_table_errors);
    UNIT_TEST(test_send_settings_ack);
    UNIT_TEST(test_send_settings_ack_errors);
    UNIT_TEST(test_check_for_settings_ack);
    UNIT_TEST(test_check_for_settings_ack_errors);
    UNIT_TEST(test_handle_settings_payload);
    UNIT_TEST(test_handle_settings_payload_errors);
    UNIT_TEST(test_read_frame);
    UNIT_TEST(test_read_frame_error);
    UNIT_TEST(test_h2_send_local_settings);
    UNIT_TEST(test_h2_read_setting_from);
    UNIT_TEST(test_h2_client_init_connection);
    UNIT_TEST(test_h2_server_init_connection);
    UNIT_TEST(test_check_incoming_headers_condition);
    UNIT_TEST(test_check_incoming_headers_condition_error);
    UNIT_TEST(test_check_incoming_headers_condition_creation_of_stream);
    UNIT_TEST(test_check_incoming_headers_condition_mismatch);
    UNIT_TEST(test_check_incoming_continuation_condition);
    UNIT_TEST(test_check_incoming_continuation_condition_errors);
    UNIT_TEST(test_handle_headers_payload_no_flags);
    UNIT_TEST(test_handle_headers_payload_just_end_stream_flag);
    UNIT_TEST(test_handle_headers_payload_full_message_header_no_end_stream);
    UNIT_TEST(test_handle_headers_payload_full_message_header_end_stream);
    UNIT_TEST(test_handle_headers_payload_errors);
    UNIT_TEST(test_handle_continuation_payload_no_end_headers_flag_set);
    UNIT_TEST(test_handle_continuation_payload_end_headers_flag_set);
    UNIT_TEST(test_handle_continuation_payload_end_headers_end_stream_flag_set);
    UNIT_TEST(test_handle_continuation_payload_errors);
    return UNIT_TESTS_END();
}
