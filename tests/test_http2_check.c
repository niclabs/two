#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"  // for h2states_t
#include "http2/check.h"
#include "frames/structs.h" // for frame_header_t
#include "cbuf.h"           // for cbuf


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(uint32_t, read_setting_from, h2states_t *, uint8_t, uint8_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);

#define FFF_FAKES_LIST(FAKE)            \
    FAKE(read_setting_from)             \
    FAKE(send_connection_error)         \
    FAKE(is_flag_set)                   \

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);
    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();

}

/* Tests http2/check functions */
void test_check_incoming_data_condition(void)
{
    cbuf_t buf_out;
    h2states_t h2s;
    frame_header_t head;

    head.length = 128;
    head.stream_id = 2440;
    h2s.waiting_for_end_headers_flag = 0;
    h2s.current_stream.stream_id = 2440;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.header = head;

    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_data_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(read_setting_from_fake.call_count == 1, "call count must be 1");

}


void test_check_incoming_data_condition_errors(void)
{
    cbuf_t buf_out;
    int rc;
    uint32_t read_setting_from_returns[1] = { 128 };

    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    // 0 test: flag waiting_for_HEADERS_frame must not
    h2states_t h2s_waiting_headers;

    h2s_waiting_headers.waiting_for_HEADERS_frame = 1;
    rc = check_incoming_data_condition(&buf_out, &h2s_waiting_headers);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (headers frame expected)");

    // 1st test: end_headers flag has not been received
    h2states_t h2s_flag;

    h2s_flag.waiting_for_end_headers_flag = 1;
    rc = check_incoming_data_condition(&buf_out, &h2s_flag);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (continuation or headers frame expected)");

    // 2nd test: Stream id is 0
    h2states_t h2s_idzero;

    h2s_idzero.waiting_for_end_headers_flag = 0;
    h2s_idzero.current_stream.stream_id = 0;
    rc = check_incoming_data_condition(&buf_out, &h2s_idzero);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (data stream id = 0)");

    // 3rd test: length of frame bigger than allowed
    frame_header_t head_len;
    h2states_t h2s;

    head_len.length = 256;
    head_len.stream_id = 2440;
    h2s.waiting_for_end_headers_flag = 0;
    h2s.current_stream.stream_id = 2440;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.header = head_len;
    rc = check_incoming_data_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (payload bigger than allowed)");

    // 4th test: Header with stream id greater than id in current stream
    frame_header_t head_idgt;

    head_idgt.length = 128;
    head_idgt.stream_id = 2442;
    h2s.header = head_idgt;
    rc = check_incoming_data_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (stream id is invalid)");

    // 5th test: Header with stream id lower than id in current stream
    frame_header_t head_idlt;

    head_idlt.length = 128;
    head_idlt.stream_id = 2438;
    h2s.header = head_idlt;
    rc = check_incoming_data_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (stream id invalid STREAM_CLOSED_ERROR)");

    // 6th test: Stream in IDLE
    frame_header_t head;
    h2states_t h2s_idle;

    head.length = 128;
    head.stream_id = 2440;
    h2s_idle.waiting_for_end_headers_flag = 0;
    h2s_idle.current_stream.stream_id = 2440;
    h2s_idle.current_stream.state = STREAM_IDLE;
    h2s_idle.header = head;
    rc = check_incoming_data_condition(&buf_out, &h2s_idle);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (stream in IDLE state, protocl)");

    // 7th test: STREAM_CLOSED_ERROR
    h2states_t h2s_closed;

    h2s_closed.waiting_for_end_headers_flag = 0;
    h2s_closed.current_stream.stream_id = 2440;
    h2s_closed.current_stream.state = STREAM_CLOSED;
    h2s_closed.header = head;
    rc = check_incoming_data_condition(&buf_out, &h2s_closed);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, (stream closed error)");

}

void test_check_incoming_headers_condition(void)
{
    cbuf_t buf_out;
    frame_header_t head;
    h2states_t h2s;
    h2states_t h2s_waiting_headers;

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
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s.waiting_for_HEADERS_frame == 0, "waiting_for_HEADERS_frame must be 0");

    h2s_waiting_headers.is_server = 0;
    h2s_waiting_headers.waiting_for_HEADERS_frame = 1;
    h2s_waiting_headers.waiting_for_end_headers_flag = 0;
    h2s_waiting_headers.current_stream.stream_id = 2440;
    h2s_waiting_headers.current_stream.state = STREAM_OPEN;
    h2s_waiting_headers.header = head;
    rc = check_incoming_headers_condition(&buf_out, &h2s_waiting_headers);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_waiting_headers.current_stream.stream_id == 2440, "Stream id must be 2440");
    TEST_ASSERT_MESSAGE(h2s_waiting_headers.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_waiting_headers.waiting_for_HEADERS_frame == 0, "waiting_for_HEADERS_frame must be 0");

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
    h2s_last.current_stream.state = STREAM_IDLE;
    h2s_last.header = head_ngt;

    h2states_t h2s_parity;
    frame_header_t head_parity;
    head_parity.stream_id = 120;
    head_parity.length = 100;
    h2s_parity.is_server = 1;
    h2s_parity.waiting_for_end_headers_flag = 0;
    h2s_parity.last_open_stream_id = 2;
    h2s_parity.current_stream.state = STREAM_IDLE;
    h2s_parity.header = head_parity;

    h2states_t h2s_parity_c;
    frame_header_t head_parity_c;
    head_parity_c.stream_id = 17;
    head_parity_c.length = 100;
    h2s_parity_c.is_server = 0;
    h2s_parity_c.waiting_for_end_headers_flag = 0;
    h2s_parity_c.last_open_stream_id = 3;
    h2s_parity_c.current_stream.state = STREAM_IDLE;
    h2s_parity_c.header = head_parity_c;

    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (waiting for end headers flag set)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_valid);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream id equals to 0)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_fs);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (frame size bigger than MAX_FRAME_SIZE");
    rc = check_incoming_headers_condition(&buf_out, &h2s_last);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream id not bigger than last open)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_parity);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream id parity is wrong)");
    rc = check_incoming_headers_condition(&buf_out, &h2s_parity_c);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream id parity is wrong)");
}

void test_check_incoming_headers_condition_creation_of_stream(void)
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
    h2s.last_open_stream_id = 2438;
    h2s.header = head;

    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Current stream id must be 2440");

    h2s.current_stream.stream_id = 2438;
    h2s.current_stream.state = STREAM_IDLE;
    rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Stream id must be 2440");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2440, "Current stream id must be 2440");
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
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (streams ids do not match)");
    TEST_ASSERT_MESSAGE(h2s.current_stream.stream_id == 2438, "Stream id must be 2438");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "Stream state must be STREAM_OPEN");

    h2s.current_stream.state = STREAM_CLOSED;
    rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (STREAM_CLOSED_ERROR)");

    h2s.current_stream.stream_id = 2440;
    rc = check_incoming_headers_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (STREAM_CLOSED_ERROR)");
}

void test_check_incoming_settings_condition(void)
{
    h2states_t h2s;
    cbuf_t buf_out;

    h2s.wait_setting_ack = 1;
    frame_header_t header_not_ack = { 24, 0x4, 0x0, 0x0, 0, NULL, NULL };
    frame_header_t header_ack = { 0, 0x4, 0x0 | 0x1, 0x0, 0, NULL, NULL };

    int flag_returns[3] = { 0, 1, 1 };
    SET_RETURN_SEQ(is_flag_set, flag_returns, 3);
    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    h2s.header = header_not_ack;
    int rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "RC must be HTTP2_RC_NO_ERROR. ACK flag is not setted");
    TEST_ASSERT_MESSAGE(h2s.wait_setting_ack == 1, "wait must remain in 1");

    h2s.header = header_ack;
    rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(is_flag_set_fake.call_count == 2, "is flag set must be called for second time");
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_ACK_RECEIVED, "RC must be HTTP2_RC_ACK_RECEIVED. ACK flag was setted and payload size was 0");
    TEST_ASSERT_MESSAGE(h2s.wait_setting_ack == 0, "wait must be changed to 0");

    rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_ACK_RECEIVED, "RC must be HTTP2_RC_ACK_RECEIVED. ACK flag was setted, but not wait ack flag asigned");
}

void test_check_incoming_settings_condition_errors(void)
{
    h2states_t h2s;
    cbuf_t buf_out;

    int i;

    for (i = 0; i < 6; i++) {
        h2s.local_settings[i] = 1;
        h2s.remote_settings[i] = 1;
    }
    h2s.wait_setting_ack = 1;
    // first error, wrong stream_id
    frame_header_t header_ack_wrong_stream = { 0, 0x4, 0x0 | 0x1, 0x1, 0, NULL, NULL };
    header_ack_wrong_stream.stream_id = 1;
    // second error, wrong size
    frame_header_t header_ack_wrong_size = { 24, 0x4, 0x0 | 0x1, 0x0, 0, NULL, NULL };

    int flag_returns[1] = { 1 };
    SET_RETURN_SEQ(is_flag_set, flag_returns, 1);
    uint32_t read_setting_from_returns[2] = { 10, 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 2);

    h2s.header = header_ack_wrong_stream;
    int rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (wrong stream)");
    h2s.header = header_ack_wrong_size;
    rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (header length > MAX_FRAME_SIZE)");
    TEST_ASSERT_MESSAGE(read_setting_from_fake.call_count == 1, "read_setting_from must be called for the first time");
    // header length < MAX_FRAME_SIZE, but its 24 != 0
    rc = check_incoming_settings_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (header length != 0)");
}

void test_check_incoming_goaway_condition(void)
{
    cbuf_t buf_out;
    h2states_t h2s;
    frame_header_t head;

    head.length = 64;
    head.type = 0x7;
    head.stream_id = 0;
    h2s.header = head;

    uint32_t read_setting_from_returns[1] = { 128 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    int rc = check_incoming_goaway_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");

}

void test_check_incoming_goaway_condition_errors(void)
{
    cbuf_t buf_out;

    uint32_t read_setting_from_returns[1] = { 128 };

    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);

    // 1st test: header with invalid id
    h2states_t h2s_id;
    frame_header_t head_id;

    head_id.stream_id = 24;
    head_id.length = 128;
    h2s_id.header = head_id;
    int rc = check_incoming_goaway_condition(&buf_out, &h2s_id);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (header with invalid id)");
    TEST_ASSERT_MESSAGE(read_setting_from_fake.call_count == 0, "call count must be 0");

    // 2nd test: header with invalid length
    h2states_t h2s_len;
    frame_header_t head_len;

    head_len.stream_id = 0;
    head_len.length = 256;
    h2s_len.header = head_len;
    rc = check_incoming_goaway_condition(&buf_out, &h2s_len);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (max frame size error)");
    TEST_ASSERT_MESSAGE(read_setting_from_fake.call_count == 1, "call count must be 1");

}

void test_check_incoming_window_update_condition(void)
{
    cbuf_t buf_out;
    h2states_t h2s_conn;
    h2states_t h2s_stream;

    h2s_conn.header.length = 4;
    h2s_conn.header.stream_id = 0;

    h2s_stream.header.length = 4;
    h2s_stream.header.stream_id = 8;
    h2s_stream.current_stream.stream_id = 8;
    h2s_stream.current_stream.state = STREAM_OPEN;

    int rc = check_incoming_window_update_condition(&buf_out, &h2s_conn);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    rc = check_incoming_window_update_condition(&buf_out, &h2s_stream);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
}

void test_check_incoming_window_update_condition_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s_len;
    h2states_t h2s_stream;

    h2s_len.header.length = 6;
    h2s_len.header.stream_id = 0;

    h2s_stream.header.length = 4;
    h2s_stream.header.stream_id = 8;
    h2s_stream.current_stream.stream_id = 8;
    h2s_stream.current_stream.state = STREAM_IDLE;

    int rc = check_incoming_window_update_condition(&buf_out, &h2s_len);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (length error)");
    rc = check_incoming_window_update_condition(&buf_out, &h2s_stream);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream idle error)");
}

void test_check_incoming_ping_condition(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.header.stream_id = 0;
    h2s.header.length = 8;
    int rc = check_incoming_ping_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
}

void test_check_incoming_ping_condition_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s_id;
    h2states_t h2s_length;

    h2s_id.header.stream_id = 24;
    h2s_id.header.length = 8;

    h2s_length.header.stream_id = 0;
    h2s_length.header.length = 10;

    int rc = check_incoming_ping_condition(&buf_out, &h2s_id);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (wrong stream id)");

    rc = check_incoming_ping_condition(&buf_out, &h2s_length);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (wrong length)");
}

void test_check_incoming_continuation_condition(void)
{
    frame_header_t head;
    h2states_t h2s;
    cbuf_t buf_out;

    head.stream_id = 440;
    head.length = 120;
    h2s.current_stream.stream_id = 440;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.waiting_for_end_headers_flag = 1;
    h2s.waiting_for_HEADERS_frame = 0;
    h2s.header = head;

    uint32_t read_setting_from_returns[1] = { 280 };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 1);
    int rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
}

void test_check_incoming_continuation_condition_errors(void)
{
    frame_header_t head;
    h2states_t h2s;
    cbuf_t buf_out;

    head.stream_id = 0;
    head.length = 290;
    h2s.current_stream.stream_id = 440;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.waiting_for_end_headers_flag = 1;
    h2s.waiting_for_HEADERS_frame = 1;
    h2s.header = head;
    uint32_t read_setting_from_returns[4] = { 280, 280, 280, DEFAULT_MAX_FRAME_SIZE };
    SET_RETURN_SEQ(read_setting_from, read_setting_from_returns, 4);

    int rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (headers frame was expected)");

    h2s.waiting_for_HEADERS_frame = 0;
    h2s.waiting_for_end_headers_flag = 0;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (not previous headers)");

    h2s.waiting_for_end_headers_flag = 1;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (incoming id equals 0 - invalid stream)");

    h2s.header.stream_id = 442;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream mistmatch - invalid stream)");

    h2s.header.stream_id = 440;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (header length bigger than max - FRAME_SIZE_ERROR)");

    h2s.header.length = 180;
    h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream not open)");

    h2s.current_stream.state = STREAM_IDLE;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream in idle)");

    h2s.current_stream.state = STREAM_OPEN;
    h2s.header_block_fragments_pointer = 128;
    h2s.header.length = HTTP2_MAX_HBF_BUFFER - 100;
    rc = check_incoming_continuation_condition(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (block fragments too big)");

}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_check_incoming_data_condition);
    UNIT_TEST(test_check_incoming_data_condition_errors);
    UNIT_TEST(test_check_incoming_headers_condition);
    UNIT_TEST(test_check_incoming_headers_condition_error);
    UNIT_TEST(test_check_incoming_headers_condition_creation_of_stream);
    UNIT_TEST(test_check_incoming_headers_condition_mismatch);
    UNIT_TEST(test_check_incoming_settings_condition);
    UNIT_TEST(test_check_incoming_settings_condition_errors);
    UNIT_TEST(test_check_incoming_goaway_condition);
    UNIT_TEST(test_check_incoming_goaway_condition_errors);
    UNIT_TEST(test_check_incoming_window_update_condition);
    UNIT_TEST(test_check_incoming_window_update_condition_errors);
    UNIT_TEST(test_check_incoming_ping_condition);
    UNIT_TEST(test_check_incoming_ping_condition_errors);
    UNIT_TEST(test_check_incoming_continuation_condition);
    UNIT_TEST(test_check_incoming_continuation_condition_errors);

    return UNIT_TESTS_END();
}
