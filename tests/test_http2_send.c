#include "unit.h"
#include "logging.h"
#include "fff.h"
#include "http2/structs.h"
#include "http2/send.h"
#include "frames/common.h"
#include "cbuf.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, create_goaway_frame, frame_header_t *, goaway_payload_t *, uint8_t *, uint32_t, uint32_t,  uint8_t *, uint8_t);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t *, uint8_t *);
FAKE_VOID_FUNC(prepare_new_stream, h2states_t *);
FAKE_VALUE_FUNC(uint8_t, set_flag, uint8_t, uint8_t);

#define FFF_FAKES_LIST(FAKE)            \
    FAKE(cbuf_push)                     \
    FAKE(create_goaway_frame)           \
    FAKE(frame_to_bytes)                \
    FAKE(prepare_new_stream)            \
    FAKE(set_flag)                      \


void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);
    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();

}

/* Tests http2/send functions */

void test_change_stream_state_end_stream_flag(void)
{
    cbuf_t buf_out;

    h2states_t h2s_open;

    h2s_open.current_stream.state = STREAM_OPEN;
    int rc = change_stream_state_end_stream_flag(0, &buf_out, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_REMOTE, "state must be STREAM_HALF_CLOSED_REMOTE");

    h2s_open.current_stream.state = STREAM_OPEN;
    rc = change_stream_state_end_stream_flag(1, &buf_out, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == 0, "return code must be 0");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "state must be STREAM_HALF_CLOSED_LOCAL");

    h2states_t h2s_hcr;

    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.received_goaway = 0;
    rc = change_stream_state_end_stream_flag(1, &buf_out, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 1, "prepare_new_stream must have been called");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->current_stream.state);
/*
    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.received_goaway = 1;
    rc = change_stream_state_end_stream_flag(1, &buf_out, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 1, "prepare_new_stream should not have been called");
*/

    h2states_t h2s_hcl;

    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 0;
    rc = change_stream_state_end_stream_flag(0, &buf_out, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 2, "prepare_new_stream must have been called");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->current_stream.state);
/*
    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 1;
    rc = change_stream_state_end_stream_flag(0, &buf_out, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 2, "prepare_new_stream should not have been called");
*/
}

/*

   void test_change_stream_state_end_stream_flag_close_connection(void){}

   void test_send_data(void){}

   void test_send_data_full_sending(void){}

   void test_send_data_errors(void){}

   void test_send_settings_ack(void){}

   void test_send_settings_ack_errors(void){}

   void test_send_goaway(void){}

   void test_send_goaway_errors(void){}

   void test_send_window_update(void){}

   void test_send_window_update_errors(void){}

   void test_send_headers_stream_verification(void){}

   void test_send_headers_stream_verification_errors(void){}

   void test_send_local_settings(void){}

   void test_send_local_settings_errors(void){}

   void test_send_continuation_frame(void){}

   void test_send_continuation_frame_errors(void){}

   void test_send_headers_frame(void){}

   void test_send_headers_frame_all_branches(void){}

   void test_send_headers_frame_errors(void){}

   void test_send_headers_one_header(void){}

   void test_send_headers_with_continuation(void){}

   void test_send_headers_errors(void){}

   void test_send_response(void){}

   void test_send_response_errors(void){}

 */

void test_example(void)
{
    TEST_FAIL();
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_change_stream_state_end_stream_flag);

    return UNIT_TESTS_END();
}
