#include <errno.h>
#include "unit.h"
#include "http2.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, verify_setting, uint16_t, uint32_t);
FAKE_VALUE_FUNC(int, read_n_bytes, uint8_t *, int);
FAKE_VALUE_FUNC(int, http_write, uint8_t *, int);
FAKE_VALUE_FUNC(int, create_settings_ack_frame, frame_t *, frame_header_t*);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t*, uint8_t*);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, bytes_to_settings_payload, uint8_t*, int, settings_payload_t*, settings_pair_t*);
FAKE_VALUE_FUNC(int, bytes_to_frame_header, uint8_t*, int , frame_header_t*);
FAKE_VALUE_FUNC(int, create_settings_frame ,uint16_t*, uint32_t*, int, frame_t*, frame_header_t*, settings_payload_t*, settings_pair_t*);

#define FFF_FAKES_LIST(FAKE)         \
    FAKE(verify_setting)             \
    FAKE(read_n_bytes)               \
    FAKE(http_write)                 \
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
void setUp(void){
  /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
}


void test_send_local_settings(void){
  /*Depends on create_settings_frame, frame_to_bytes and http_write*/
  h2states_t dummy = {{1,1,1,1,1,1},
                      {1,1,1,1,1,1},
                      0};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  http_write_fake.custom_fake = return_24;
  int rc = send_local_settings(&dummy);
  TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
  TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame_to_bytes must be called once");
  TEST_ASSERT_MESSAGE(http_write_fake.call_count == 1, "http_write must be called once");
  TEST_ASSERT_MESSAGE(dummy.wait_setting_ack == 1, "wait_setting_ack must be 1. Settings were sent");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of send_local_settings must be 0");
}

void test_client_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  h2states_t client;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  http_write_fake.custom_fake = return_24;
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
  TEST_ASSERT_MESSAGE(http_write_fake.call_count == 2, "http_write must be called twice");
  TEST_ASSERT_MESSAGE(rc == 0, "return code of client_init_connection must be 0");
}


void test_server_init_connection(void){
  /*Depends on http_write and send_local_settings*/
  h2states_t server;
  uint32_t init_vals[6] = {DEFAULT_HTS,DEFAULT_EP,DEFAULT_MCS,DEFAULT_IWS,DEFAULT_MFS,DEFAULT_MHLS};
  create_settings_frame_fake.custom_fake = return_zero;
  frame_to_bytes_fake.custom_fake = return_24;
  http_write_fake.custom_fake = return_24;
  read_n_bytes_fake.custom_fake = return_24;
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
  TEST_ASSERT_MESSAGE(http_write_fake.call_count == 0, "http_write must be called once");
  TEST_ASSERT_MESSAGE(rc==0, "return code of client_init_connection must be 0");
}



int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_send_local_settings);
    UNIT_TEST(test_client_init_connection);
    UNIT_TEST(test_server_init_connection);
    return UNIT_TESTS_END();
}
