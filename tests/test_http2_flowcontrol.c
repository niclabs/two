#include "unit.h"
#include "logging.h"

#include "http2/structs.h"
#include "http2/flowcontrol.h"

void test_get_window_available_size(void)
{
    // Create function parameters
    h2_window_manager_t wm;

    wm.window_used = 5;
    wm.window_size = 15;

    // Perform request
    uint32_t size = get_window_available_size(wm);

    // Return value should be 10
    TEST_ASSERT_EQUAL(10, size);
}


void test_increase_window_used(void)
{
    // Create function parameters
    h2_window_manager_t wm;

    wm.window_used = 5;

    // Perform request
    int iwu = increase_window_used(&wm, 3);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, iwu);

    // Check if window_used have the correct content
    TEST_ASSERT_EQUAL( 8, wm.window_used);
}


void test_decrease_window_used(void)
{
    // Create function parameters
    h2_window_manager_t wm;

    wm.window_used = 15;

    // Perform request
    int dwu = decrease_window_used(&wm, 13);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, dwu);

    // Check if window_used have the correct content
    TEST_ASSERT_EQUAL( 2, wm.window_used);
}


void test_flow_control_receive_data_success(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.incoming_window.window_size = 15;
    h2s.incoming_window.window_used = 5;

    // Perform request
    int fcrd = flow_control_receive_data(&h2s, 7);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, fcrd);

    // Check if incoming_window have the correct content
    TEST_ASSERT_EQUAL( 12, h2s.incoming_window.window_used);
}


void test_flow_control_receive_data_fail(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.incoming_window.window_size = 15;
    h2s.incoming_window.window_used = 10;

    // Perform request
    int fcrd = flow_control_receive_data(&h2s, 7);

    // Return value should be -1
    TEST_ASSERT_EQUAL(HTTP2_RC_ERROR, fcrd);

    // Check if incoming_window have the correct content
    TEST_ASSERT_EQUAL( 10, h2s.incoming_window.window_used);
}


void test_flow_control_send_data_success(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.outgoing_window.window_size = 15;
    h2s.outgoing_window.window_used = 5;

    // Perform request
    int fcsd = flow_control_send_data(&h2s, 7);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, fcsd);

    // Check if outgoing_window have the correct content
    TEST_ASSERT_EQUAL( 12, h2s.outgoing_window.window_used);
}


void test_flow_control_send_data_fail(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.outgoing_window.window_size = 15;
    h2s.outgoing_window.window_used = 10;

    // Perform request
    int fcsd = flow_control_send_data(&h2s, 7);

    // Return value should be -1
    TEST_ASSERT_EQUAL(HTTP2_RC_ERROR, fcsd);

    // Check if outgoing_window have the correct content
    TEST_ASSERT_EQUAL( 10, h2s.outgoing_window.window_used);
}


void test_flow_control_send_window_update_success(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.incoming_window.window_used = 7;

    // Perform request
    int swu = flow_control_send_window_update(&h2s, 5);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, swu);

    // Check if outgoing_window have the correct content
    TEST_ASSERT_EQUAL( 2, h2s.incoming_window.window_used);
}


void test_flow_control_send_window_update_fail(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.incoming_window.window_used = 5;

    // Perform request
    int swu = flow_control_send_window_update(&h2s, 7);

    // Return value should be -1
    TEST_ASSERT_EQUAL(HTTP2_RC_ERROR, swu);

    // Check if outgoing_window have the correct content
    TEST_ASSERT_EQUAL( 5, h2s.incoming_window.window_used);
}


void test_flow_control_receive_window_update_success_id0(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.header.stream_id = 0;
    h2s.remote_window.connection_window = 10;

    // Perform request
    int rwu = flow_control_receive_window_update(&h2s, 7);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, rwu);

    // Check if connection_window have the correct content
    TEST_ASSERT_EQUAL( 17, h2s.remote_window.connection_window);
}


void test_flow_control_receive_window_update_success(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.header.stream_id = 5;
    h2s.current_stream.stream_id = 5;
    h2s.remote_window.stream_window = 10;

    // Perform request
    int rwu = flow_control_receive_window_update(&h2s, 7);

    // Return value should be 0
    TEST_ASSERT_EQUAL(HTTP2_RC_NO_ERROR, rwu);

    // Check if connection_window have the correct content
    TEST_ASSERT_EQUAL( 17, h2s.remote_window.stream_window);
}



void test_flow_control_receive_window_update_fail(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.header.stream_id = 0;
    h2s.remote_window.connection_window = -8;

    // Perform request
    int rwu = flow_control_receive_window_update(&h2s, 7);

    // Return value should be -1
    TEST_ASSERT_EQUAL(HTTP2_RC_ERROR, rwu);

    // Check if outgoing_window have the correct content
    TEST_ASSERT_EQUAL( -1, h2s.remote_window.connection_window);
}


void test_get_size_data_to_send_v1(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.outgoing_window.window_size = 18;
    h2s.outgoing_window.window_used = 0;
    h2s.data.size = 20;
    h2s.data.processed = 15;

    // Perform request
    int gsds = get_size_data_to_send(&h2s);

    // Return value should be 5
    TEST_ASSERT_EQUAL(5, gsds);
}


void test_get_size_data_to_send_v2(void)
{
    // Create function parameters
    h2states_t h2s;

    h2s.outgoing_window.window_size = 18;
    h2s.outgoing_window.window_used = 10;
    h2s.data.size = 20;
    h2s.data.processed = 0;

    // Perform request
    int gsds = get_size_data_to_send(&h2s);

    // Return value should be 8
    TEST_ASSERT_EQUAL(8, gsds);
}



int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_get_window_available_size);
    UNIT_TEST(test_increase_window_used);
    UNIT_TEST(test_decrease_window_used);

    UNIT_TEST(test_flow_control_receive_data_success);
    UNIT_TEST(test_flow_control_receive_data_fail);

    UNIT_TEST(test_flow_control_send_data_success);
    UNIT_TEST(test_flow_control_send_data_fail);

    UNIT_TEST(test_flow_control_send_window_update_success);
    UNIT_TEST(test_flow_control_send_window_update_fail);

    UNIT_TEST(test_flow_control_receive_window_update_success_id0);
    UNIT_TEST(test_flow_control_receive_window_update_success);
    UNIT_TEST(test_flow_control_receive_window_update_fail);

    UNIT_TEST(test_get_size_data_to_send_v1);
    UNIT_TEST(test_get_size_data_to_send_v2);


    return UNIT_TESTS_END();
}
