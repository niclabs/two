#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2utils.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, http_read, hstates_t*, uint8_t*, int);


#define FFF_FAKES_LIST(FAKE)        \
    FAKE(http_read)

void setUp(void) {
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_read_n_bytes(void){
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  hstates_t st;
  // http_read reading rate is 10 bytes
  int http_return[5] = {10, 10, 10, 10, 10};
  SET_RETURN_SEQ(http_read, http_return, 5  );
  int rc = read_n_bytes(buff, 50, &st);
  TEST_ASSERT_MESSAGE(rc == 50, "rc must be 50");
  TEST_ASSERT_MESSAGE(http_read_fake.call_count == 5, "call count must be 5");
}

void test_read_n_bytes_error(void){
  uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
  hstates_t st;
  // http_read reading rate is 10 bytes
  int http_return[5] = {10, 10, 10, 10, 0};
  SET_RETURN_SEQ(http_read, http_return, 5);
  int rc = read_n_bytes(buff, 50, &st);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (http_read error)");
  TEST_ASSERT_MESSAGE(http_read_fake.call_count == 5, "call count must be 5");
}

void test_prepare_new_stream(void){
    hstates_t st;
    st.h2s.last_open_stream_id = 333;
    st.is_server = 1;
    int rc = prepare_new_stream(&st);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 334, "Stream id must be 334");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_IDLE,"Stream state must be STREAM_IDLE");
    st.is_server = 0;
    rc = prepare_new_stream(&st);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.stream_id == 335, "Stream id must be 335");
    TEST_ASSERT_MESSAGE(st.h2s.current_stream.state == STREAM_IDLE,"Stream state must be STREAM_IDLE");
}

void test_read_setting_from(void){
  hstates_t hdummy;
  int i;
  for(i = 0; i < 6; i++){
    hdummy.h2s.remote_settings[i] = i+1;
    hdummy.h2s.local_settings[i] = i+7;
  }
  uint32_t answ = read_setting_from(&hdummy, LOCAL, 0x1);
  TEST_ASSERT_MESSAGE(answ == 7, "Answer must be 7");
  answ = read_setting_from(&hdummy, REMOTE, 0x6);
  TEST_ASSERT_MESSAGE(answ == 6, "Answer must be 6");
  answ = read_setting_from(&hdummy, LOCAL, 0x0);
  TEST_ASSERT_MESSAGE((int)answ == -1, "Answer must be -1. Error in id! (overvalue)");
  answ = read_setting_from(&hdummy, REMOTE, 0x7);
  TEST_ASSERT_MESSAGE((int)answ == -1, "Answer mus be -1. Error in id! (uppervalue)");
}

void test_read_setting_from_errors(void){
  hstates_t hdummy;
  int i;
  for(i = 0; i < 6; i++){
    hdummy.h2s.remote_settings[i] = i+1;
    hdummy.h2s.local_settings[i] = i+7;
  }
  // First error, invalid parameter
  uint32_t rc = read_setting_from(&hdummy, LOCAL, 0x0);
  TEST_ASSERT_MESSAGE((int)rc == -1, "rc must be -1 (invalid parameter");
  rc = read_setting_from(&hdummy, 5, 0x1);
  TEST_ASSERT_MESSAGE((int)rc == -1, "rc must be -1 (invalid table");
}



int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_read_n_bytes);
    UNIT_TEST(test_read_n_bytes_error);
    UNIT_TEST(test_prepare_new_stream);
    UNIT_TEST(test_read_setting_from);
    UNIT_TEST(test_read_setting_from_errors);
    return UNIT_TESTS_END();
}
