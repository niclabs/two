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

void test_verify_setting(void){
  int rc;
  rc = verify_setting(HEADER_TABLE_SIZE, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for HTS");
  rc = verify_setting(ENABLE_PUSH, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for EP");
  rc = verify_setting(ENABLE_PUSH, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for EP");
  rc = verify_setting(MAX_CONCURRENT_STREAMS, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for MCS");
  rc = verify_setting(INITIAL_WINDOW_SIZE, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for IWS");
  rc = verify_setting(MAX_CONCURRENT_STREAMS, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for MCS");
  rc = verify_setting(MAX_FRAME_SIZE, 16384);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for MFS");
  rc = verify_setting(MAX_FRAME_SIZE, 16777214);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for MFS");
  rc = verify_setting(MAX_HEADER_LIST_SIZE, 1);
  TEST_ASSERT_MESSAGE(rc == 0, "rc must be 0, valid value was given for MHLS");
  rc = verify_setting(0x20, 1);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1, invalid parameter");
}

void test_verify_setting_errors(void){
  int rc;
  rc = verify_setting(0x7, 3);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (invalid id)");
  rc = verify_setting(HEADER_TABLE_SIZE, 0);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (invalid header table size)");
  rc = verify_setting(ENABLE_PUSH, 2);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (invalid enable push)");
  rc = verify_setting(MAX_CONCURRENT_STREAMS, 0);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (invalid max concurretn streams)");
  rc = verify_setting(INITIAL_WINDOW_SIZE, 2147483648);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (value greater than max for initial window size)");
  rc = verify_setting(MAX_FRAME_SIZE, 16382);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (value lower than min for max frame size)");
  rc = verify_setting(MAX_FRAME_SIZE, 16777216);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (value greater than max for max frame size)");
  rc = verify_setting(MAX_HEADER_LIST_SIZE, 0);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (value equals zero for max_header_list_size)");
  rc = verify_setting(0x9, 0);
  TEST_ASSERT_MESSAGE(rc == -1, "rc must be -1 (invalid id)");
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

void test_get_setting_value(void){
    uint32_t settings[6];
    settings[0] = 0;
    settings[1] = 123123;
    settings[2] = 1234566;
    settings[3] = 16;
    settings[4] = 123;
    settings[5] = 987432958;

    uint32_t value_received;
    value_received = get_setting_value(settings, HEADER_TABLE_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[0]);
    value_received = get_setting_value(settings, ENABLE_PUSH);
    TEST_ASSERT_EQUAL(value_received, settings[1]);
    value_received = get_setting_value(settings, MAX_CONCURRENT_STREAMS);
    TEST_ASSERT_EQUAL(value_received, settings[2]);
    value_received = get_setting_value(settings, INITIAL_WINDOW_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[3]);
    value_received = get_setting_value(settings, MAX_FRAME_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[4]);
    value_received = get_setting_value(settings, MAX_HEADER_LIST_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[5]);
}

void test_get_header_list_size(void){

    header_t table_pair[2];
    strcpy(table_pair[0].name, "name1");//5
    strcpy(table_pair[0].value, "value1");//6
    //table_pair[0].value = "value1";
    //table_pair[1].name = "anothername";
    strcpy(table_pair[1].name, "anothername");//11
    strcpy(table_pair[1].value, "anothervalue");//12
    //table_pair[1].value = "anothervalue";

    headers_t headers;
    headers.count = 2;
    headers.maxlen = 2;
    headers.headers = table_pair;

    uint32_t expected_size =(uint32_t) strlen(table_pair[0].name) + strlen(table_pair[0].value) + 64 + strlen(table_pair[1].name) + strlen(table_pair[1].value) + 64;//adds 32 for the overhead of every header name and value (check RFC)

    uint32_t result = get_header_list_size(&headers);
    TEST_ASSERT_EQUAL(result, expected_size);
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_verify_setting);
    UNIT_TEST(test_verify_setting_errors);
    UNIT_TEST(test_read_n_bytes);
    UNIT_TEST(test_read_n_bytes_error);
    UNIT_TEST(test_get_setting_value);
    UNIT_TEST(test_get_header_list_size);

    return UNIT_TESTS_END();
}
