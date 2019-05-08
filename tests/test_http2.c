#include <errno.h>
#include "unit.h"
#include "http2.h"
#include "fff.h"
 /*---------- Import of functions not declared in http2.h ----------------*/
extern int init_variables(h2states_t * st);
extern int update_settings_table(settings_payload_t *spl, uint8_t place, h2states_t *st);
extern int send_settings_ack(void);
extern int read_settings_payload(uint8_t *buff_read, frame_header_t *header, h2states_t *st);
extern int read_frame(uint8_t *buff_read, frame_header_t *header);
 /*---------------- Mock functions ---------------------------*/

 uint8_t buffer[MAX_BUFFER_SIZE];
 int size = 0;

 int read_n_bytes(uint8_t *buff_read, int n){
   int read_bytes = 0;
   int incoming_bytes;
   while(read_bytes < n){
     incoming_bytes = http_read(buff_read+read_bytes, n - read_bytes);
     /* incoming_bytes equals -1 means that there was an error*/
     if(incoming_bytes == -1){
       puts("Error in read function");
       return -1;
     }
     read_bytes = read_bytes + incoming_bytes;
   }
   return read_bytes;
 }

 // Toy write function
 int http_write(uint8_t *bytes, int length){
   if(size+length > MAX_BUFFER_SIZE){
     return -1;
   }
   memcpy(buffer+size, bytes, length);
   size += length;
   printf("Write: buffer size is %u\n", size);
   return length;
 }

// Toy read function
int http_read(uint8_t *bytes, int length){
  if(length > size){
      length = size;
  }
  //Write to caller
  memcpy(bytes, buffer, length);
  size = size - length;
  //Move the rest of the data on buffer
  memcpy(buffer, buffer+length, size);
  printf("Read: buffer size is %u\n", size);
  return length;
}


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, verify_setting, uint16_t, uint32_t);
FAKE_VALUE_FUNC(int, create_settings_ack_frame, frame_t *, frame_header_t*);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t*, uint8_t*);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, bytes_to_settings_payload, uint8_t*, int, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, bytes_to_frame_header, uint8_t*, int , frame_header_t*);
FAKE_VALUE_FUNC(int, create_settings_frame ,uint16_t*, uint32_t*, int, frame_t*, frame_header_t*, settings_payload_t*, settings_pair_t*);

#define FFF_FAKES_LIST(FAKE)         \
    FAKE(verify_setting)             \
    FAKE(create_settings_ack_frame)  \
    FAKE(frame_to_bytes)             \
    FAKE(is_flag_set)                \
    FAKE(bytes_to_settings_payload)  \
    FAKE(bytes_to_frame_header)      \
    FAKE(create_settings_frame)

/*----------Value Return for FAKEs ----------*/
int return_zero(void){
  return 0;
}

int return_negative(void){
  return -1;
}

int return_24(void){
  return 24;
}

/*---------- TESTING SUIT --------------------*/
void setUp(void){
  /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
  size = 0;
}

void test_init_variables(void){
  h2states_t dummy;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
  int rc = init_variables(&dummy);
  TEST_ASSERT_MESSAGE(dummy.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.wait_setting_ack == 0, "WAIT musy be 0");
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
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  verify_setting_fake.custom_fake = return_zero;
  int rc = update_settings_table(&payload, LOCAL, &dummy);
  TEST_ASSERT_MESSAGE(verify_setting_fake.call_count == 6, "Call count of verify_setting must be 6");
  TEST_ASSERT_MESSAGE(dummy.local_settings[0] == 12345, "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[1] == 0, "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[2] == 12345, "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[3] == 12345, "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[4] == 12345, "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[5] == 12345, "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[0] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[1] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[2] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[3] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[4] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[5] == 1, "Remote settings were not modified!");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  rc = update_settings_table(&payload, REMOTE, &dummy);
  TEST_ASSERT_MESSAGE(verify_setting_fake.call_count == 12, "Call count of verify_setting must be 6");
  TEST_ASSERT_MESSAGE(dummy.local_settings[0] == 12345, "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[1] == 0, "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[2] == 12345, "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[3] == 12345, "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[4] == 12345, "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.local_settings[5] == 12345, "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[0] == 12345, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[1] == 0, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[2] == 12345, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[3] == 12345, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[4] == 12345, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(dummy.remote_settings[5] == 12345, "Remote settings must be modified!");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");

}

void test_send_settings_ack(void){
  int rc = send_settings_ack();
  create_settings_ack_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count = 1, "ACK creation not called");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count = 1, "frame to bytes not called");
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
}

void test_read_settings_payload(void){
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      1};
  frame_header_t header_ack = {0, 0x4, 0x0|0x1, 0x0, 0};
  frame_header_t header_sett = {24, 0x4, 0x0, 0x0, 0};
  int flag_returns[2] = {1, 0};
  SET_RETURN_SEQ(is_flag_set, flag_returns, 2);
  bytes_to_settings_payload_fake.custom_fake = return_24;
  create_settings_ack_frame_fake.custom_fake = return_zero;
  verify_setting_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  int rc = read_settings_payload(buffer, &header_ack, &dummy);
  TEST_ASSERT_MESSAGE(rc == 0, "RC must be 0");
  TEST_ASSERT_MESSAGE(is_flag_set_fake.call_count == 1, "is_flag_set must be called once");
  TEST_ASSERT_MESSAGE(dummy.wait_setting_ack == 0, "ACK wait must be 0");
  /*rc = read_settings_payload(buffer, &header_sett, &dummy);*/
}

void test_send_local_settings(void){
  /*Depends on create_settings_frame, frame_to_bytes and http_write*/
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  int rc = send_local_settings(&dummy);
  TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame_to_bytes must be called once");
  TEST_ASSERT_MESSAGE(dummy.wait_setting_ack == 1, "wait_setting_ack must be 1. Settings were sent");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of send_local_settings must be 0");
}

void test_client_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  h2states_t client;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  int rc = client_init_connection(&client);
  TEST_ASSERT_MESSAGE(client.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(client.remote_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of client_init_connection must be 0");
}

void test_server_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  h2states_t server;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  uint8_t preface_buff[24];
  uint8_t i = 0;
  /*We load the fake buffer with the preface*/
  while(preface[i] != '\0'){
    preface_buff[i] = preface[i];
    i++;
  }
  int wrc = http_write(preface_buff,24);
  TEST_ASSERT_MESSAGE(wrc == 24, "Fake buffer not written");
  int rc = server_init_connection(&server);
  TEST_ASSERT_MESSAGE(server.local_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.local_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.local_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.local_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.local_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.local_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[0] == init_vals[0], "HTS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[1] == init_vals[1], "EP in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[2] == init_vals[2], "MCS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[3] == init_vals[3], "IWS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[4] == init_vals[4], "MFS in local settings is not setted");
  TEST_ASSERT_MESSAGE(server.remote_settings[5] == init_vals[5], "MHLS in local settings is not setted");
  TEST_ASSERT_MESSAGE(rc==0, "return code of server_init_connection must be 0");
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_init_variables);
    UNIT_TEST(test_update_settings_table);
    UNIT_TEST(test_send_settings_ack);
    UNIT_TEST(test_read_settings_payload);
    UNIT_TEST(test_send_local_settings);
    UNIT_TEST(test_client_init_connection);
    UNIT_TEST(test_server_init_connection);
    return UNIT_TESTS_END();
}
