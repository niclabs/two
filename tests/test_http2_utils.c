#include "unit.h"
#include "logging.h"

#include "http2/utils.h"
#include "http2/structs.h"

void test_prepare_new_stream_server_odd(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.is_server = 1;
    h2s.last_open_stream_id = 5;

    // Perform request
    prepare_new_stream(&h2s);

    // Check if current_stream have the correct content
    TEST_ASSERT_EQUAL( 6, h2s.current_stream.stream_id);
    TEST_ASSERT_EQUAL( STREAM_IDLE, h2s.current_stream.state);
}


void test_prepare_new_stream_client_even(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.is_server = 0;
    h2s.last_open_stream_id = 10;

    // Perform request
    prepare_new_stream(&h2s);

    // Check if current_stream have the correct content
    TEST_ASSERT_EQUAL( 11, h2s.current_stream.stream_id);
    TEST_ASSERT_EQUAL( STREAM_IDLE, h2s.current_stream.state);
}


void test_read_setting_from_local(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.local_settings[4] = 4;

    // Perform request
    int rsf = read_setting_from(&h2s, 0, 5);

    // Check return value
    TEST_ASSERT_EQUAL( h2s.local_settings[4], rsf);
}


void test_read_setting_from_remote(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.remote_settings[3] = 24;

    // Perform request
    int rsf = read_setting_from(&h2s, 1, 4);

    // Check return value
    TEST_ASSERT_EQUAL( h2s.remote_settings[3], rsf);
}


void test_read_setting_from_fail_invalid_param(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.remote_settings[3] = 24;

    // Perform request
    int rsf = read_setting_from(&h2s, 0, 14);

    // Check return value
    TEST_ASSERT_EQUAL( -1, rsf);
}


void test_read_setting_from_fail_invalid_place(void) {
    // Create function parameters
    h2states_t h2s;

    h2s.remote_settings[3] = 24;

    // Perform request
    int rsf = read_setting_from(&h2s, 2, 4);

    // Check return value
    TEST_ASSERT_EQUAL( -1, rsf);
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_prepare_new_stream_server_odd);
    UNIT_TEST(test_prepare_new_stream_client_even);

    UNIT_TEST(test_read_setting_from_local);
    UNIT_TEST(test_read_setting_from_remote);
    UNIT_TEST(test_read_setting_from_fail_invalid_param);
    UNIT_TEST(test_read_setting_from_fail_invalid_place);

    return UNIT_TESTS_END();
}
