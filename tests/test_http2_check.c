#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"  // for h2states_t
#include "http2/check.h"
#include "frames/common.h"  // for frame_header_t
#include "cbuf.h"           // for cbuf

/* Import of functions not declared in http2/check.h */
extern uint32_t read_setting_from(h2states_t *st, uint8_t place, uint8_t param);
extern void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(uint32_t, read_setting_from, h2states_t *, uint8_t, uint8_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);

#define FFF_FAKES_LIST(FAKE)            \
    FAKE(read_setting_from)             \
    FAKE(send_connection_error)         \

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);
    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();

}
void test_check_incoming_headers_condition(void)
{
    cbuf_t buf_out;
    frame_header_t head;
    h2states_t h2s;

    head.stream_id = 2440;
    head.length = 128;
    h2s.is_server = 0;
    h2s.waiting_for_end_headers_flag = 0;
    h2s.current_stream.stream_id = 2440;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.header = head;
    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    int rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
}

void test_check_incoming_headers_condition_error(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.waiting_for_end_headers_flag = 1;

    h2states_t h2s_valid;
    frame_header_t head_invalid;
    head_invalid.stream_id = 0;
    h2s_valid.waiting_for_end_headers_flag = 0;
    h2s_valid.header = head_invalid;

    h2states_t h2s_fs;
    frame_header_t head_fs;
    head_fs.stream_id = 12;
    head_fs.length = 200;
    h2s_fs.waiting_for_end_headers_flag = 0;
    h2s_fs.header = head_fs;

    h2states_t h2s_last;
    frame_header_t head_ngt;
    head_ngt.stream_id = 123;
    head_ngt.length = 100;
    h2s_last.waiting_for_end_headers_flag = 0;
    h2s_last.last_open_stream_id = 124;
    h2s_last.header = head_ngt;

    h2states_t h2s_parity;
    frame_header_t head_parity;
    head_parity.stream_id = 120;
    head_parity.length = 100;
    h2s_parity.is_server = 1;
    h2s_parity.waiting_for_end_headers_flag = 0;
    h2s_parity.last_open_stream_id = 2;
    h2s_parity.header = head_parity;

    h2states_t h2s_parity_c;
    frame_header_t head_parity_c;
    head_parity_c.stream_id = 17;
    h2s_parity_c.is_server = 0;
    h2s_parity_c.waiting_for_end_headers_flag = 0;
    h2s_parity_c.last_open_stream_id = 3;
    h2s_parity_c.header = head_parity_c;

    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (waiting for end headers flag set)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_valid);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id equals to 0)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_fs);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (frame size bigger than MAX_FRAME_SIZE");
    rc = check_incoming_headers_condition(&buf_out, &h2s_last);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id not bigger than last open)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_parity);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id parity is wrong)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_parity_c);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (stream id parity is wrong)");
}
void test_check_incoming_headers_condition_mismatch(void)
{
    cbuf_t buf_out;
    frame_header_t head;
    h2states_t h2s;

    head.stream_id = 2440;
    head.length = 256;
    h2s.waiting_for_end_headers_flag = 0;
    h2s.current_stream.stream_id = 2438;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.header = head;

    uint32_t read_setting_from_returns[1] = { 280 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (streams ids do not match)");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2438, "Stream id must be 2438");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

    h2s.current_stream.state = STREAM_CLOSED;
    rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (STREAM_CLOSED_ERROR)");

    h2s.current_stream.stream_id = 2440;
    rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == -1, "Return code must be -1 (STREAM_CLOSED_ERROR)");
}
int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_check_incoming_headers_condition);
    UNIT_TEST(test_check_incoming_headers_condition_error);
    UNIT_TEST(test_check_incoming_headers_condition_mismatch);

    return UNIT_TESTS_END();
}
