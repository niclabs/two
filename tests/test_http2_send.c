#include "unit.h"
#include "logging.h"
#include "fff.h"
#include "http2/structs.h"
#include "http2/send.h"
#include "frames/structs.h"
#include "headers.h"
#include "cbuf.h"

extern int send_headers_stream_verification(h2states_t *h2s);
extern int send_continuation_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_stream, h2states_t *h2s);
extern int send_headers_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, uint8_t end_stream, h2states_t *h2s);

typedef unsigned int uint;

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, event_write, event_sock_t *, uint, uint8_t *, event_write_cb);
FAKE_VALUE_FUNC(int, event_read_stop_and_write, event_sock_t *, uint, uint8_t *, event_write_cb);
FAKE_VOID_FUNC(http2_on_read_continue, event_sock_t *, int);
FAKE_VOID_FUNC(create_goaway_frame, frame_header_t *, goaway_payload_t *, uint8_t *, uint32_t, uint32_t,  uint8_t *, uint8_t);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t *, uint8_t *);
FAKE_VOID_FUNC(prepare_new_stream, h2states_t *);
FAKE_VALUE_FUNC(uint8_t, set_flag, uint8_t, uint8_t);
FAKE_VOID_FUNC(create_data_frame, frame_header_t *, data_payload_t *, uint8_t *, uint8_t *, int, uint32_t);
FAKE_VALUE_FUNC(uint32_t, get_size_data_to_send, h2states_t *);
FAKE_VALUE_FUNC(int, flow_control_send_data, h2states_t *, uint32_t);
FAKE_VOID_FUNC(create_settings_ack_frame, frame_t *, frame_header_t *);
FAKE_VALUE_FUNC(int, create_window_update_frame, frame_header_t *, window_update_payload_t *, int, uint32_t);
FAKE_VALUE_FUNC(int, flow_control_send_window_update, h2states_t *, uint32_t);
FAKE_VOID_FUNC(create_continuation_frame, uint8_t *, int, uint32_t, frame_header_t *, continuation_payload_t *, uint8_t *);
FAKE_VOID_FUNC(create_settings_frame, uint16_t *, uint32_t *, int, frame_header_t *, settings_payload_t *, settings_pair_t *);
FAKE_VOID_FUNC(create_headers_frame, uint8_t *, int, uint32_t, frame_header_t *, headers_payload_t *, uint8_t *);
FAKE_VOID_FUNC(create_ping_ack_frame, frame_header_t *, ping_payload_t *, uint8_t *);
FAKE_VOID_FUNC(create_ping_frame, frame_header_t *, ping_payload_t *, uint8_t *);
FAKE_VALUE_FUNC(int, compress_headers, header_list_t *, uint8_t *, hpack_states_t *);
FAKE_VALUE_FUNC(int, read_setting_from, h2states_t *, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, headers_count, header_list_t *);
FAKE_VALUE_FUNC(uint32_t, get_sending_available_window, h2states_t *);
FAKE_VOID_FUNC(decrease_window_available, h2_flow_control_window_t *, uint32_t );

#define FFF_FAKES_LIST(FAKE)                \
    FAKE(cbuf_push)                         \
    FAKE(event_write)                       \
    FAKE(event_read_stop_and_write)         \
    FAKE(http2_on_read_continue)            \
    FAKE(create_goaway_frame)               \
    FAKE(frame_to_bytes)                    \
    FAKE(prepare_new_stream)                \
    FAKE(set_flag)                          \
    FAKE(create_data_frame)                 \
    FAKE(get_size_data_to_send)             \
    FAKE(flow_control_send_data)            \
    FAKE(create_settings_ack_frame)         \
    FAKE(create_window_update_frame)        \
    FAKE(flow_control_send_window_update)   \
    FAKE(create_settings_frame)             \
    FAKE(create_continuation_frame)         \
    FAKE(create_headers_frame)              \
    FAKE(create_ping_ack_frame)             \
    FAKE(create_ping_frame)                 \
    FAKE(compress_headers)                  \
    FAKE(read_setting_from)                 \
    FAKE(headers_count)                     \
    FAKE(get_sending_available_window)      \
    FAKE(decrease_window_available)         \

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

    h2states_t h2s_open;

    h2s_open.current_stream.state = STREAM_OPEN;
    int rc = change_stream_state_end_stream_flag(0, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_REMOTE, "state must be STREAM_HALF_CLOSED_REMOTE");

    h2s_open.current_stream.state = STREAM_OPEN;
    rc = change_stream_state_end_stream_flag(1, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == 0, "return code must be 0");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "state must be STREAM_HALF_CLOSED_LOCAL");

    h2states_t h2s_hcr;

    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.received_goaway = 0;
    rc = change_stream_state_end_stream_flag(1, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 1, "prepare_new_stream must have been called");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->current_stream.state);

    h2states_t h2s_hcl;

    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 0;
    rc = change_stream_state_end_stream_flag(0, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 2, "prepare_new_stream must have been called");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->current_stream.state);

}

void test_change_stream_state_end_stream_flag_close_connection(void)
{

    h2states_t h2s_hcr;

    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.received_goaway = 1;
    int rc = change_stream_state_end_stream_flag(1, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 0, "prepare_new_stream should not have been called");

    h2states_t h2s_hcl;

    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 1;
    rc = change_stream_state_end_stream_flag(0, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 0, "prepare_new_stream should not have been called");

}

/*
void test_send_data(void)
{
    h2states_t h2s;
    http2_data_t data;

    data.size = 36;
    data.processed = 0;
    h2s.outgoing_window.window_size = 30;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.data = data;

    get_size_data_to_send_fake.return_val = 30;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "stream state must remain as STREAM_OPEN");
    TEST_ASSERT_EQUAL(0, set_flag_fake.call_count);
}

void test_send_data_end_stream(void)
{
    http2_data_t data;
    h2states_t h2s_open;

    data.size = 36;
    data.processed = 0;
    h2s_open.outgoing_window.window_size = 30;
    h2s_open.current_stream.state = STREAM_OPEN;
    h2s_open.current_stream.stream_id = 24;
    h2s_open.data = data;

    get_size_data_to_send_fake.return_val = 30;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(1, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s_open.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "stream state must be STREAM_HALF_CLOSED_LOCAL");
    TEST_ASSERT_EQUAL(1, set_flag_fake.call_count);

    h2states_t h2s_hcr;

    h2s_hcr.outgoing_window.window_size = 30;
    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.current_stream.stream_id = 24;
    h2s_hcr.received_goaway = 0;
    h2s_hcr.data = data;

    rc = send_data(1, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcr.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s_hcr.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "stream state must be STREAM_CLOSED");
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);

}

void test_send_data_full_sending(void)
{
    http2_data_t data;
    h2states_t h2s;

    data.size = 36;
    data.processed = 0;
    h2s.outgoing_window.window_size = 50;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.data = data;

    get_size_data_to_send_fake.return_val = 36;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 0, "data processed must be 0");
    TEST_ASSERT_MESSAGE(h2s.data.size == 0, "data size must be 0 ");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "stream state must remain as STREAM_OPEN");

}
*/


void test_send_data_errors(void)
{
    h2states_t h2s_nodata;

    h2s_nodata.data.size = 0;
    h2s_nodata.data.processed = 0;

    int rc = send_data(0, &h2s_nodata);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "no data to send");

    h2states_t h2s_wstate;

    h2s_wstate.data.size = 32;
    h2s_wstate.data.processed = 0;
    h2s_wstate.current_stream.state = STREAM_CLOSED;

    rc = send_data(0, &h2s_wstate);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "wrong state for sending data");

    get_size_data_to_send_fake.return_val = 32;

    h2states_t h2s_write;
    h2s_write.data.size = 32;
    h2s_write.data.processed = 0;
    h2s_write.current_stream.state = STREAM_OPEN;

    frame_to_bytes_fake.return_val = 39;
    int push_return[2] = { 30, 39 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    rc = send_data(0, &h2s_write);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "error writing data frame");

    h2states_t h2s_fc;

    h2s_fc.data.size = 32;
    h2s_fc.data.processed = 0;
    h2s_fc.current_stream.state = STREAM_OPEN;

    flow_control_send_data_fake.return_val = HTTP2_RC_ERROR;

    rc = send_data(0, &h2s_fc);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Trying to send more data than allowed");
    TEST_ASSERT_EQUAL(32, flow_control_send_data_fake.arg1_val);
}

/*
void test_send_data_close_connection(void)
{
    h2states_t h2s;

    h2s.outgoing_window.window_size = 30;
    h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s.current_stream.stream_id = 24;
    h2s.received_goaway = 1;
    h2s.data.size = 36;
    h2s.data.processed = 0;

    get_size_data_to_send_fake.return_val = 36;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 0, "value of data processed must remain as 0"); // review
    TEST_ASSERT_MESSAGE(h2s.data.size == 36, "value of data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_CLOSED, "stream state must be STREAM_CLOSED");

}
*/

void test_send_settings_ack(void)
{
    h2states_t h2s;

    frame_to_bytes_fake.return_val = 9;
    event_write_fake.return_val = 9;

    int rc = send_settings_ack(&h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "RC must be 0");
    TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count == 1, "ACK creation called one time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes called one time");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write called one time");


}

void test_send_settings_ack_errors(void)
{
    h2states_t h2s;

    frame_to_bytes_fake.return_val = 9;

    int push_return[2] = { 4, 9 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    int rc = send_settings_ack(&h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "RC must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, error sending");
    TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count == 1, "ACK creation called one time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes is called when sending goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write is called when sending goaway");
    TEST_ASSERT_EQUAL(9, event_write_fake.arg1_val);

}

void test_send_ping(void)
{
    uint8_t opaque_data[8];
    h2states_t h2s;

    frame_to_bytes_fake.return_val = 17;
    event_write_fake.return_val = 17;

    int rc = send_ping(opaque_data, 0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_ping_frame_fake.call_count == 1, "create_ping_frame called one time");
    TEST_ASSERT_MESSAGE(create_ping_ack_frame_fake.call_count == 0, "create_ping_ack_frame called 0 times");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes is called one time");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write is called one time");
    TEST_ASSERT_EQUAL(17, event_write_fake.arg1_val);
}

void test_send_ping_ack(void)
{
    uint8_t opaque_data[8];
    h2states_t h2s;

    frame_to_bytes_fake.return_val = 17;
    event_write_fake.return_val = 17;

    int rc = send_ping(opaque_data, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_ping_frame_fake.call_count == 0, "create_ping_frame called 0 times");
    TEST_ASSERT_MESSAGE(create_ping_ack_frame_fake.call_count == 1, "create_ping_ack_frame called 1 time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes is called one time");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write is called one time");
    TEST_ASSERT_EQUAL(17, event_write_fake.arg1_val);
}


void test_send_ping_errors(void)
{
    uint8_t opaque_data[8];
    h2states_t h2s;

    frame_to_bytes_fake.return_val = 17;
    int push_return[2] = { 8, 17 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    int rc = send_ping(opaque_data, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending)");
    TEST_ASSERT_MESSAGE(create_ping_frame_fake.call_count == 0, "create_ping_frame called 0 times");
    TEST_ASSERT_MESSAGE(create_ping_ack_frame_fake.call_count == 1, "create_ping_ack_frame called 1 time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes is called in send_ping and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write is called in send_ping and send_goaway");
    TEST_ASSERT_EQUAL(17, event_write_fake.arg1_val);
}

void test_send_goaway(void)
{
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 0;
    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_goaway(HTTP2_PROTOCOL_ERROR, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "RC must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.sent_goaway == 1, "sent_goaway must be 1");
    TEST_ASSERT_EQUAL(32, event_write_fake.arg1_val);


}

void test_send_goaway_close_connection(void)
{
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 1;

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_goaway(HTTP2_NO_ERROR, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "RC must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s.sent_goaway == 1, "sent_goaway must be 1");
    TEST_ASSERT_EQUAL(32, event_write_fake.arg1_val);
}

void test_send_goaway_errors(void)
{
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 1;

    frame_to_bytes_fake.return_val = 8;
    event_write_fake.return_val = 2;

    int rc = send_goaway(HTTP2_NO_ERROR, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_ERROR, "rc must be HTTP2_RC_ERROR (writing frame)");

}

/*
void test_send_window_update(void)
{
    h2states_t h2s;

    h2s.incoming_window.window_used = 24;

    frame_to_bytes_fake.return_val = 32;
    cbuf_push_fake.return_val = 32;
    flow_control_send_window_update_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_window_update(16, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(16, flow_control_send_window_update_fake.arg1_val);
}
*/

/*
void test_send_window_update_errors(void)
{
    h2states_t h2s;

    h2s.incoming_window.window_used = 24;

    int create_wu_return[4] = { -1, 0, 0, 0 };
    SET_RETURN_SEQ(create_window_update_frame, create_wu_return, 4);

    frame_to_bytes_fake.return_val = 13;

    int push_return[6] = { 13, 13, 5, 13, 13, 13 };
    SET_RETURN_SEQ(cbuf_push, push_return, 6);

    flow_control_send_window_update_fake.return_val = HTTP2_RC_ERROR;

    int rc = send_window_update(28, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error creating frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes must be called by send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 1, "cbuf_push must be called by send_goaway");

    rc = send_window_update(28, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (window increment too big)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes must be called by send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 2, "cbuf_push must be called by send_goaway");

    rc = send_window_update(16, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error writing frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 4, "frame to bytes must be called by send_window_update and send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 4, "cbuf_push must be called by send_window_update and send_goaway");

    rc = send_window_update(16, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 6, "frame to bytes must be called by send_window_update and send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 6, "cbuf_push must be called by send_window_update and send_goaway");


}
*/

void test_send_headers_stream_verification_server(void)
{
    h2states_t h2s_idle;

    h2s_idle.is_server = 1;
    h2s_idle.current_stream.stream_id = 2;
    h2s_idle.current_stream.state = STREAM_IDLE;
    h2s_idle.last_open_stream_id = 1;

    h2states_t h2s_parity;

    h2s_parity.is_server = 1;
    h2s_parity.current_stream.stream_id = 4;
    h2s_parity.current_stream.state = STREAM_IDLE;
    h2s_parity.last_open_stream_id = 4;

    h2states_t h2s_id_gt;

    h2s_id_gt.is_server = 1;
    h2s_id_gt.current_stream.stream_id = 4;
    h2s_id_gt.current_stream.state = STREAM_IDLE;
    h2s_id_gt.last_open_stream_id = 25;

    h2states_t h2s_open;

    h2s_open.is_server = 1;
    h2s_open.current_stream.stream_id = 24;
    h2s_open.current_stream.state = STREAM_OPEN;
    h2s_open.last_open_stream_id = 24;

    int rc = send_headers_stream_verification(&h2s_idle);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.stream_id == 2, "stream_id of the new stream must be 2");
    TEST_ASSERT_MESSAGE(h2s_idle.last_open_stream_id == 2, "last open stream id must be 2");

    rc = send_headers_stream_verification(&h2s_parity);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.stream_id == 6, "stream_id of the new stream must be 6");
    TEST_ASSERT_MESSAGE(h2s_parity.last_open_stream_id == 6, "last open stream id must be 6");

    rc = send_headers_stream_verification(&h2s_id_gt);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.stream_id == 26, "stream_id of the new stream must be 26");
    TEST_ASSERT_MESSAGE(h2s_id_gt.last_open_stream_id == 26, "last open stream id must be 26");

    rc = send_headers_stream_verification(&h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_OPEN, "stream state must remain STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.stream_id == 24, "stream_id of the new stream must be 24");
    TEST_ASSERT_MESSAGE(h2s_open.last_open_stream_id == 24, "last open stream id must be 24");

}

void test_send_headers_stream_verification_client(void)
{
    h2states_t h2s_idle;

    h2s_idle.is_server = 0;
    h2s_idle.current_stream.stream_id = 3;
    h2s_idle.current_stream.state = STREAM_IDLE;
    h2s_idle.last_open_stream_id = 1;

    h2states_t h2s_parity;

    h2s_parity.is_server = 0;
    h2s_parity.current_stream.stream_id = 5;
    h2s_parity.current_stream.state = STREAM_IDLE;
    h2s_parity.last_open_stream_id = 4;

    h2states_t h2s_id_gt;

    h2s_id_gt.is_server = 0;
    h2s_id_gt.current_stream.stream_id = 5;
    h2s_id_gt.current_stream.state = STREAM_IDLE;
    h2s_id_gt.last_open_stream_id = 25;

    h2states_t h2s_open;

    h2s_open.is_server = 0;
    h2s_open.current_stream.stream_id = 23;
    h2s_open.current_stream.state = STREAM_OPEN;
    h2s_open.last_open_stream_id = 23;

    int rc = send_headers_stream_verification(&h2s_idle);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.stream_id == 3, "stream_id of the new stream must be 3");
    TEST_ASSERT_MESSAGE(h2s_idle.last_open_stream_id == 3, "last open stream id must be 3");

    rc = send_headers_stream_verification(&h2s_parity);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.stream_id == 5, "stream_id of the new stream must be 5");
    TEST_ASSERT_MESSAGE(h2s_parity.last_open_stream_id == 5, "last open stream id must be 5");

    rc = send_headers_stream_verification(&h2s_id_gt);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.stream_id == 27, "stream_id of the new stream must be 27");
    TEST_ASSERT_MESSAGE(h2s_id_gt.last_open_stream_id == 27, "last open stream id must be 27");

    rc = send_headers_stream_verification(&h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_OPEN, "stream state must remain STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.stream_id == 23, "stream_id of the new stream must be 23");
    TEST_ASSERT_MESSAGE(h2s_open.last_open_stream_id == 23, "last open stream id must be 23");
}

void test_send_headers_stream_verification_errors(void)
{
    h2states_t h2s_closed;
    h2states_t h2s_hcl;

    h2s_closed.current_stream.state = STREAM_CLOSED;
    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_headers_stream_verification(&h2s_closed);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream closed)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write_fake must be called in send_goaway");

    rc = send_headers_stream_verification(&h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream closed)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write_fake must be called in send_goaway");

}

void test_send_local_settings(void)
{
    h2states_t h2s;

    for (int i = 0; i < 6; i++) {
        h2s.local_settings[i] = 1;
        h2s.remote_settings[i] = 1;
    }

    frame_to_bytes_fake.return_val = 16;
    event_write_fake.return_val = 16;

    int rc = send_local_settings(&h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.wait_setting_ack == 1, "wait setting ack must be 1");
    TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame_to_bytes must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 16, "size_byte_mysettings must be 16");
}

void test_send_local_settings_errors(void)
{
    h2states_t h2s;

    h2s.wait_setting_ack = 0;
    for (int i = 0; i < 6; i++) {
        h2s.local_settings[i] = 1;
        h2s.remote_settings[i] = 1;
    }

    frame_to_bytes_fake.return_val = 16;

    int push_return[2] = { 4, 16 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    int rc = send_local_settings(&h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error writing)");
    TEST_ASSERT_MESSAGE(h2s.wait_setting_ack == 0, "wait setting ack must be 0");
    TEST_ASSERT_MESSAGE(create_settings_frame_fake.call_count == 1, "create_settings_frame must be called once");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame_to_bytes must be called once in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write must be called once in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 16, "size_byte_mysettings must be 16");

}

void test_send_continuation_frame(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_continuation_frame(buff, 32, 5, 0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_continuation_frame_fake.call_count == 1, "Create continuation frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 0, "Set flag call count must be 0");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes call count must be 1");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write call count must be 1");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");

}

void test_send_continuation_frame_end_headers(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_continuation_frame(buff, 32, 5, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "Return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_continuation_frame_fake.call_count == 1, "Create continuation frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 1, "Set flag call count must be 1");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes call count must be 1");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write call count must be 1");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");

}

void test_send_continuation_frame_errors(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
    int rc;

    frame_to_bytes_fake.return_val = 32;
    int push_return[2] = { 16, 32 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    rc = send_continuation_frame(buff, 32, 5, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error writing)");
    TEST_ASSERT_MESSAGE(create_continuation_frame_fake.call_count == 1, "Create continuation frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 1, "Set flag call count must be 1");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "Frame to bytes must be called in send_continuation_frame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write call must be called in send_continuation_frame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");

}

void test_send_headers_frame(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_headers_frame(buff, 32, 5, 0, 0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "create_headers_frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 0, "set_flag call count must be 0");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");

}

void test_send_headers_frame_end_headers(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_headers_frame(buff, 32, 5, 1, 0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "create_headers_frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 1, "set_flag call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.arg1_val == HEADERS_END_HEADERS_FLAG, "set_flag arg1 must be HEADERS_END_HEADERS_FLAG");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");

}

void test_send_headers_frame_end_stream(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_headers_frame(buff, 32, 5, 0, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "create_headers_frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 1, "set_flag call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.arg1_val == HEADERS_END_STREAM_FLAG, "set_flag arg1 must be HEADERS_END_STREAM_FLAG");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");
}

void test_send_headers_frame_all_branches(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];

    frame_to_bytes_fake.return_val = 32;
    event_write_fake.return_val = 32;

    int rc = send_headers_frame(buff, 32, 5, 1, 1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "create_headers_frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 2, "set_flag call count must be 1");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write must be called once");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");
}

void test_send_headers_frame_errors(void)
{
    h2states_t h2s;
    uint8_t buff[HTTP2_MAX_BUFFER_SIZE];
    int rc;

    frame_to_bytes_fake.return_val = 32;
    int push_return[2] = { 16, 32 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    rc = send_headers_frame(buff, 32, 5, 0, 0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Return code must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error writing)");
    TEST_ASSERT_MESSAGE(create_headers_frame_fake.call_count == 1, "Create headers frame call count must be 1");
    TEST_ASSERT_MESSAGE(set_flag_fake.call_count == 0, "Set flag call count must be 0");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "Frame to bytes must be called in send_headers_frame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write call must be called in send_headers_frame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.arg1_val == 32, "event_write arg2 must be 32");
}

void test_send_headers_one_header(void)
{
    h2states_t h2s;
    h2states_t h2s_es;

    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.is_server = 1;
    h2s.last_open_stream_id = 24;
    h2s.headers.n_entries = 10;
    h2s.received_goaway = 0;

    h2s_es.current_stream.state = STREAM_OPEN;
    h2s_es.current_stream.stream_id = 24;
    h2s_es.is_server = 1;
    h2s_es.last_open_stream_id = 24;
    h2s_es.headers.n_entries = 10;
    h2s_es.received_goaway = 0;

    headers_count_fake.return_val = 10;
    compress_headers_fake.return_val = 20;
    read_setting_from_fake.return_val = 20;

    // with end_stream
    int rc = send_headers(1, &h2s_es);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);
    TEST_ASSERT_EQUAL(STREAM_HALF_CLOSED_LOCAL, h2s_es.current_stream.state);
    // no end stream
    rc = send_headers(0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(2, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(3, set_flag_fake.call_count);
    TEST_ASSERT_EQUAL(STREAM_OPEN, h2s.current_stream.state);

}

void test_send_headers_with_continuation(void)
{
    h2states_t h2s_es;
    h2states_t h2s;

    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.is_server = 1;
    h2s.last_open_stream_id = 24;
    h2s.headers.n_entries = 10;
    h2s.received_goaway = 0;

    h2s_es.current_stream.state = STREAM_OPEN;
    h2s_es.current_stream.stream_id = 24;
    h2s_es.is_server = 1;
    h2s_es.last_open_stream_id = 24;
    h2s_es.headers.n_entries = 10;
    h2s_es.received_goaway = 0;

    headers_count_fake.return_val = 10;
    compress_headers_fake.return_val = 200; // size
    read_setting_from_fake.return_val = 20; // max frame size

    // with end_stream
    int rc = send_headers(1, &h2s_es);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(9, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);
    TEST_ASSERT_EQUAL(STREAM_HALF_CLOSED_LOCAL, h2s_es.current_stream.state);

    // no end_stream
    rc = send_headers(0, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(2, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(18, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(3, set_flag_fake.call_count);
    TEST_ASSERT_EQUAL(STREAM_OPEN, h2s.current_stream.state);
}

void test_send_headers_errors(void)
{
    h2states_t h2s_ga;
    h2states_t h2s_zero;
    h2states_t h2s_cver;

    h2s_ga.received_goaway = 1;

    h2s_zero.received_goaway = 0;
    h2s_zero.headers.n_entries = 0;

    h2s_cver.received_goaway = 0;
    h2s_cver.headers.n_entries = 20;
    h2s_cver.current_stream.state = STREAM_CLOSED;

    int headers_return[3] = { 0, 10, 10 };
    SET_RETURN_SEQ(headers_count, headers_return, 3);

    int compress_return[4] = { -1, 20 };
    SET_RETURN_SEQ(compress_headers, compress_return, 2);

    int rc = send_headers(1, &h2s_ga);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (goaway received)");
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "Frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 1, "event_write call must be called in send_goaway");

    rc = send_headers(1, &h2s_zero);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (no headers)");
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_count_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "Frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write call must be called in send_goaway");

    rc = send_headers(1, &h2s_cver);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error compressing)");
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(1, compress_headers_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 3, "Frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 3, "event_write call must be called in send_goaway");

    rc = send_headers(1, &h2s_cver);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream error in send_..._verification)");
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(3, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(2, compress_headers_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 4, "Frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 4, "event_write call must be called in send_goaway");
}

void test_send_headers_errors_one_header(void)
{
    h2states_t h2s;

    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.is_server = 1;
    h2s.last_open_stream_id = 24;
    h2s.headers.n_entries = 10;
    h2s.received_goaway = 0;

    headers_count_fake.return_val = 10;
    compress_headers_fake.return_val = 20;
    read_setting_from_fake.return_val = 20;
    frame_to_bytes_fake.return_val = 128;
    int push_return[2] = { 64, 128 };
    SET_RETURN_SEQ(event_write, push_return, 2);

    int rc = send_headers(1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending)");
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "Frame to bytes must be called in send_headers_fame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write call must be called in send_headers_fame and send_goaway");
}

void test_send_headers_errors_with_continuation(void)
{
    h2states_t h2s;

    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.is_server = 1;
    h2s.last_open_stream_id = 24;
    h2s.headers.n_entries = 10;
    h2s.received_goaway = 0;

    headers_count_fake.return_val = 10;
    compress_headers_fake.return_val = 60; // max_frame_size
    read_setting_from_fake.return_val = 20;
    frame_to_bytes_fake.return_val = 128;
    int push_return[9] = { 64, 128, 128, 64, 128, 128, 128, 64, 128 };
    SET_RETURN_SEQ(event_write, push_return, 9);

    int rc = send_headers(1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending)");
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, set_flag_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "Frame to bytes must be called in send_headers_fame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 2, "event_write call must be called in send_headers_fame and send_goaway");

    rc = send_headers(1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending)");
    TEST_ASSERT_EQUAL(2, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 5, "Frame to bytes must be called in send_headers_fame, send_continuation_frame and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 5, "event_write call must be called in send_headers_fame, send_continuation_frame and send_goaway");

    rc = send_headers(1, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending)");
    TEST_ASSERT_EQUAL(3, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(3, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(4, set_flag_fake.call_count);
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 9, "Frame to bytes must be called in send_headers_fame, send_continuation_frame x2 and send_goaway");
    TEST_ASSERT_MESSAGE(event_write_fake.call_count == 9, "event_write call must be called in send_headers_fame, send_continuation_frame x2 and send_goaway");
}

void test_send_response(void)
{
    h2states_t h2s_data;
    h2states_t h2s_nodata;

    h2s_data.data.size = 10;
    h2s_data.data.processed = 0;
    h2s_data.headers.n_entries = 10;
    h2s_data.received_goaway = 0;
    h2s_data.current_stream.state = STREAM_OPEN;
    h2s_data.current_stream.stream_id = 24;

    h2s_nodata.data.size = 0;
    h2s_nodata.data.processed = 0;
    h2s_nodata.headers.n_entries = 10;
    h2s_nodata.received_goaway = 0;
    h2s_nodata.current_stream.state = STREAM_OPEN;
    h2s_nodata.current_stream.stream_id = 24;

    headers_count_fake.return_val = 10;
    compress_headers_fake.return_val = 20;
    read_setting_from_fake.return_val = 20;
    get_size_data_to_send_fake.return_val = 10;

    int rc = send_response(&h2s_data);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, create_data_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);
    TEST_ASSERT_MESSAGE(h2s_data.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "stream state must be STREAM_HALF_CLOSED_LOCAL");
    TEST_ASSERT_MESSAGE(h2s_data.data.size == 0, "data size must be 0");
    TEST_ASSERT_MESSAGE(h2s_data.data.processed == 0, "data processed must be 0");

    rc = send_response(&h2s_nodata);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(2, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_continuation_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, create_data_frame_fake.call_count);
    TEST_ASSERT_EQUAL(4, set_flag_fake.call_count);
    TEST_ASSERT_MESSAGE(h2s_nodata.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "stream state must be STREAM_HALF_CLOSED_LOCAL");
    TEST_ASSERT_MESSAGE(h2s_nodata.data.size == 0, "data size must remain 0");
    TEST_ASSERT_MESSAGE(h2s_nodata.data.processed == 0, "data processed must remain 0");


}

void test_send_response_errors(void)
{
    h2states_t h2s;
    h2states_t h2s_bad_headers;
    h2states_t h2s_bad_headers_data;
    h2states_t h2s_bad_data;

    int headers_count_return[5] = { 0, 10, 10, 10, 10 };

    SET_RETURN_SEQ(headers_count, headers_count_return, 5);
    compress_headers_fake.return_val = 20;
    read_setting_from_fake.return_val = 20;
    flow_control_send_data_fake.return_val = HTTP2_RC_ERROR;

    h2s_bad_headers.data.size = 0;
    h2s_bad_headers.data.processed = 0;
    h2s_bad_headers.received_goaway = 1;

    h2s_bad_data.data.size = 20;
    h2s_bad_data.data.processed = 0;
    h2s_bad_data.received_goaway = 0;
    h2s_bad_data.headers.n_entries = 10;
    h2s_bad_data.current_stream.state = STREAM_OPEN;

    h2s_bad_headers_data.data.size = 20;
    h2s_bad_headers_data.data.processed = 0;
    h2s_bad_headers_data.received_goaway = 1;

    int rc = send_response(&h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (no headers)");
    TEST_ASSERT_EQUAL(1, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    rc = send_response(&h2s_bad_headers);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (goaway received)");
    TEST_ASSERT_EQUAL(2, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    rc = send_response(&h2s_bad_headers_data);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (goaway received)");
    TEST_ASSERT_EQUAL(3, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(0, create_headers_frame_fake.call_count);
    rc = send_response(&h2s_bad_data);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error in send_data: flowcontrol)");
    TEST_ASSERT_EQUAL(5, headers_count_fake.call_count);
    TEST_ASSERT_EQUAL(1, create_headers_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, create_data_frame_fake.call_count);
    TEST_ASSERT_EQUAL(2, set_flag_fake.call_count);
    TEST_ASSERT_EQUAL(1, flow_control_send_data_fake.call_count);

}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_change_stream_state_end_stream_flag);
    UNIT_TEST(test_change_stream_state_end_stream_flag_close_connection);
    //TODO: Fix tests in test_http2_send
    //UNIT_TEST(test_send_data);
    //UNIT_TEST(test_send_data_end_stream);
    //UNIT_TEST(test_send_data_full_sending);
    //UNIT_TEST(test_send_data_errors);
    //UNIT_TEST(test_send_data_close_connection);
    //UNIT_TEST(test_send_settings_ack);
    //UNIT_TEST(test_send_settings_ack_errors);
    //UNIT_TEST(test_send_ping);
    //UNIT_TEST(test_send_ping_ack);
    //UNIT_TEST(test_send_ping_errors);
    //UNIT_TEST(test_send_goaway);
    //UNIT_TEST(test_send_goaway_close_connection);
    UNIT_TEST(test_send_goaway_errors);
    //UNIT_TEST(test_send_window_update);
    //UNIT_TEST(test_send_window_update_errors);
    UNIT_TEST(test_send_headers_stream_verification_server);
    UNIT_TEST(test_send_headers_stream_verification_client);
    //UNIT_TEST(test_send_headers_stream_verification_errors);
    //UNIT_TEST(test_send_local_settings);
    //UNIT_TEST(test_send_local_settings_errors);
    //UNIT_TEST(test_send_continuation_frame);
    //UNIT_TEST(test_send_continuation_frame_end_headers);
    //UNIT_TEST(test_send_continuation_frame_errors);
    //UNIT_TEST(test_send_headers_frame);
    //UNIT_TEST(test_send_headers_frame_end_stream);
    //UNIT_TEST(test_send_headers_frame_end_headers);
    //UNIT_TEST(test_send_headers_frame_errors);
    UNIT_TEST(test_send_headers_one_header);
    UNIT_TEST(test_send_headers_with_continuation);
    //UNIT_TEST(test_send_headers_errors);
    //UNIT_TEST(test_send_headers_errors_one_header);
    //UNIT_TEST(test_send_headers_errors_with_continuation);
    //UNIT_TEST(test_send_response);
    //UNIT_TEST(test_send_response_errors);

    return UNIT_TESTS_END();
}
