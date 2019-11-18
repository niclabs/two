#include "unit.h"
#include "logging.h"
#include "fff.h"
#include "http2/structs.h"
#include "http2/send.h"
#include "frames/common.h"
#include "cbuf.h"

extern int send_headers_stream_verification(cbuf_t *buf_out, h2states_t *h2s);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, create_goaway_frame, frame_header_t *, goaway_payload_t *, uint8_t *, uint32_t, uint32_t,  uint8_t *, uint8_t);
FAKE_VALUE_FUNC(int, frame_to_bytes, frame_t *, uint8_t *);
FAKE_VOID_FUNC(prepare_new_stream, h2states_t *);
FAKE_VALUE_FUNC(uint8_t, set_flag, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, create_data_frame, frame_header_t *, data_payload_t *, uint8_t *, uint8_t *, int, uint32_t);
FAKE_VALUE_FUNC(uint32_t, get_size_data_to_send, h2states_t *);
FAKE_VALUE_FUNC(int, flow_control_send_data, h2states_t *, uint32_t);
FAKE_VALUE_FUNC(int, create_settings_ack_frame, frame_t *, frame_header_t *);
FAKE_VALUE_FUNC(int, create_window_update_frame, frame_header_t *, window_update_payload_t *, int, uint32_t);
FAKE_VALUE_FUNC(int, flow_control_send_window_update, h2states_t *, uint32_t);

#define FFF_FAKES_LIST(FAKE)                \
    FAKE(cbuf_push)                         \
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

    h2states_t h2s_hcl;

    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 0;
    rc = change_stream_state_end_stream_flag(0, &buf_out, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 2, "prepare_new_stream must have been called");
    TEST_ASSERT_EQUAL(STREAM_CLOSED, prepare_new_stream_fake.arg0_val->current_stream.state);

}

void test_change_stream_state_end_stream_flag_close_connection(void)
{
    cbuf_t buf_out;

    h2states_t h2s_hcr;

    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.received_goaway = 1;
    int rc = change_stream_state_end_stream_flag(1, &buf_out, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 0, "prepare_new_stream should not have been called");

    h2states_t h2s_hcl;

    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;
    h2s_hcl.received_goaway = 1;
    rc = change_stream_state_end_stream_flag(0, &buf_out, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s_hcl.current_stream.state == STREAM_CLOSED, "state must be STREAM_CLOSED");
    TEST_ASSERT_MESSAGE(prepare_new_stream_fake.call_count == 0, "prepare_new_stream should not have been called");

}

void test_send_data(void)
{
    cbuf_t buf_out;
    h2states_t h2s;
    http2_data_t data;

    data.size = 36;
    data.processed = 0;
    h2s.outgoing_window.window_size = 30;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.data = data;

    get_size_data_to_send_fake.return_val = 30;
    create_data_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(0, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "stream state must remain as STREAM_OPEN");
}

void test_send_data_end_stream(void)
{
    cbuf_t buf_out;
    http2_data_t data;
    h2states_t h2s_open;

    data.size = 36;
    data.processed = 0;
    h2s_open.outgoing_window.window_size = 30;
    h2s_open.current_stream.state = STREAM_OPEN;
    h2s_open.current_stream.stream_id = 24;
    h2s_open.data = data;

    get_size_data_to_send_fake.return_val = 30;
    create_data_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(1, &buf_out, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s_open.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_HALF_CLOSED_LOCAL, "stream state must be STREAM_HALF_CLOSED_LOCAL");

    h2states_t h2s_hcr;

    h2s_hcr.outgoing_window.window_size = 30;
    h2s_hcr.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s_hcr.current_stream.stream_id = 24;
    h2s_hcr.received_goaway = 0;
    h2s_hcr.data = data;

    rc = send_data(1, &buf_out, &h2s_hcr);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_hcr.data.processed == 30, "data processed must be 30");
    TEST_ASSERT_MESSAGE(h2s_hcr.data.size == 36, "data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s_hcr.current_stream.state == STREAM_CLOSED, "stream state must be STREAM_CLOSED");

}

void test_send_data_full_sending(void)
{
    cbuf_t buf_out;
    http2_data_t data;
    h2states_t h2s;

    data.size = 36;
    data.processed = 0;
    h2s.outgoing_window.window_size = 50;
    h2s.current_stream.state = STREAM_OPEN;
    h2s.current_stream.stream_id = 24;
    h2s.data = data;

    get_size_data_to_send_fake.return_val = 36;
    create_data_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(0, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "return code must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 0, "data processed must be 0");
    TEST_ASSERT_MESSAGE(h2s.data.size == 0, "data size must be 0 ");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_OPEN, "stream state must remain as STREAM_OPEN");

}


void test_send_data_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s_nodata;

    h2s_nodata.data.size = 0;
    h2s_nodata.data.processed = 0;

    int rc = send_data(0, &buf_out, &h2s_nodata);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "no data to send");

    h2states_t h2s_wstate;

    h2s_wstate.data.size = 32;
    h2s_wstate.data.processed = 0;
    h2s_wstate.current_stream.state = STREAM_CLOSED;

    rc = send_data(0, &buf_out, &h2s_wstate);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "wrong state for sending data");

    h2states_t h2s_create;
    h2s_create.data.size = 32;
    h2s_create.data.processed = 0;
    h2s_create.current_stream.state = STREAM_OPEN;

    get_size_data_to_send_fake.return_val = 32;
    int create_return[3] = { -1, 0, 0 };
    SET_RETURN_SEQ(create_data_frame, create_return, 3);

    rc = send_data(0, &buf_out, &h2s_create);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "error creating data frame");


    h2states_t h2s_write;
    h2s_write.data.size = 32;
    h2s_write.data.processed = 0;
    h2s_write.current_stream.state = STREAM_OPEN;

    frame_to_bytes_fake.return_val = 39;
    int push_return[2] = { 30, 39 };
    SET_RETURN_SEQ(cbuf_push, push_return, 2);

    rc = send_data(0, &buf_out, &h2s_write);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "error writing data frame");


    h2states_t h2s_fc;
    h2s_fc.data.size = 32;
    h2s_fc.data.processed = 0;
    h2s_fc.current_stream.state = STREAM_OPEN;

    flow_control_send_data_fake.return_val = HTTP2_RC_ERROR;

    rc = send_data(0, &buf_out, &h2s_fc);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "Trying to send more data than allowed");
    TEST_ASSERT_EQUAL(32, flow_control_send_data_fake.arg1_val);
}

void test_send_data_close_connection(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.outgoing_window.window_size = 30;
    h2s.current_stream.state = STREAM_HALF_CLOSED_REMOTE;
    h2s.current_stream.stream_id = 24;
    h2s.received_goaway = 1;
    h2s.data.size = 36;
    h2s.data.processed = 0;

    get_size_data_to_send_fake.return_val = 36;
    create_data_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 39;
    cbuf_push_fake.return_val = 39;
    flow_control_send_data_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_data(1, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "return code must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s.data.processed == 0, "value of data processed must remain as 0"); // review
    TEST_ASSERT_MESSAGE(h2s.data.size == 36, "value of data size must remain as 36");
    TEST_ASSERT_MESSAGE(h2s.current_stream.state == STREAM_CLOSED, "stream state must be STREAM_CLOSED");


}

void test_send_settings_ack(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    create_settings_ack_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 9;
    cbuf_push_fake.return_val = 9;

    int rc = send_settings_ack(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "RC must be 0");
    TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count == 1, "ACK creation called one time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes called one time");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 1, "cbuf_push called one time");


}

void test_send_settings_ack_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    int create_return[2] = { -1, 0 };

    SET_RETURN_SEQ(create_settings_ack_frame, create_return, 2);

    frame_to_bytes_fake.return_val = 9;

    int push_return[3] = { 9, 4, 9 };
    SET_RETURN_SEQ(cbuf_push, push_return, 3);

    int rc = send_settings_ack(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "RC must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, error creating");
    TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count == 1, "ACK creation called one time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes is called when sending goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 1, "cbuf_push is called when sending goaway");

    rc = send_settings_ack(&buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "RC must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, error writing to cbuf");
    TEST_ASSERT_MESSAGE(create_settings_ack_frame_fake.call_count == 2, "ACK creation called one time");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 3, "frame to bytes called in send settings and send goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 3, "cbuf_push called in send settings and send goaway");
    TEST_ASSERT_EQUAL(9, cbuf_push_fake.arg2_val);

}

void test_send_goaway(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 0;

    create_goaway_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 32;
    cbuf_push_fake.return_val = 32;

    int rc = send_goaway(HTTP2_PROTOCOL_ERROR, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "RC must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s.sent_goaway == 1, "sent_goaway must be 1");
    TEST_ASSERT_EQUAL(32, cbuf_push_fake.arg2_val);


}

void test_send_goaway_close_connection(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 1;

    create_goaway_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 32;
    cbuf_push_fake.return_val = 32;

    int rc = send_goaway(HTTP2_NO_ERROR, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION, "RC must be HTTP2_RC_CLOSE_CONNECTION");
    TEST_ASSERT_MESSAGE(h2s.sent_goaway == 1, "sent_goaway must be 1");
    TEST_ASSERT_EQUAL(32, cbuf_push_fake.arg2_val);
}

void test_send_goaway_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.sent_goaway = 0;
    h2s.received_goaway = 1;

    int create_return[2] = { -1, 0 };
    SET_RETURN_SEQ(create_goaway_frame, create_return, 2);

    int rc = send_goaway(HTTP2_NO_ERROR, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_ERROR, "rc must be HTTP2_RC_ERROR (creating frame)");

    frame_to_bytes_fake.return_val = 8;
    cbuf_push_fake.return_val = 2;

    rc = send_goaway(HTTP2_NO_ERROR, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_ERROR, "rc must be HTTP2_RC_ERROR (writing frame)");

}

void test_send_window_update(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.incoming_window.window_used = 24;

    create_window_update_frame_fake.return_val = 0;
    frame_to_bytes_fake.return_val = 32;
    cbuf_push_fake.return_val = 32;
    flow_control_send_window_update_fake.return_val = HTTP2_RC_NO_ERROR;

    int rc = send_window_update(16, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_EQUAL(16, flow_control_send_window_update_fake.arg1_val);
}

void test_send_window_update_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s;

    h2s.incoming_window.window_used = 24;

    int create_wu_return[4] = { -1, 0, 0, 0 };
    SET_RETURN_SEQ(create_window_update_frame, create_wu_return, 4);

    frame_to_bytes_fake.return_val = 13;

    int push_return[6] = { 13, 13, 5, 13, 13, 13 };
    SET_RETURN_SEQ(cbuf_push, push_return, 6);

    flow_control_send_window_update_fake.return_val = HTTP2_RC_ERROR;

    int rc = send_window_update(28, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error creating frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes must be called by send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 1, "cbuf_push must be called by send_goaway");

    rc = send_window_update(28, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (window increment too big)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes must be called by send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 2, "cbuf_push must be called by send_goaway");

    rc = send_window_update(16, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error writing frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 4, "frame to bytes must be called by send_window_update and send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 4, "cbuf_push must be called by send_window_update and send_goaway");

    rc = send_window_update(16, &buf_out, &h2s);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (error sending frame)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 6, "frame to bytes must be called by send_window_update and send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 6, "cbuf_push must be called by send_window_update and send_goaway");


}

void test_send_headers_stream_verification_server(void)
{
    cbuf_t buf_out;
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

    int rc = send_headers_stream_verification(&buf_out, &h2s_idle);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.stream_id == 2, "stream_id of the new stream must be 2");
    TEST_ASSERT_MESSAGE(h2s_idle.last_open_stream_id == 2, "last open stream id must be 2");

    rc = send_headers_stream_verification(&buf_out, &h2s_parity);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.stream_id == 6, "stream_id of the new stream must be 6");
    TEST_ASSERT_MESSAGE(h2s_parity.last_open_stream_id == 6, "last open stream id must be 6");

    rc = send_headers_stream_verification(&buf_out, &h2s_id_gt);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.stream_id == 26, "stream_id of the new stream must be 26");
    TEST_ASSERT_MESSAGE(h2s_id_gt.last_open_stream_id == 26, "last open stream id must be 26");

    rc = send_headers_stream_verification(&buf_out, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_OPEN, "stream state must remain STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.stream_id == 24, "stream_id of the new stream must be 24");
    TEST_ASSERT_MESSAGE(h2s_open.last_open_stream_id == 24, "last open stream id must be 24");

}

void test_send_headers_stream_verification_client(void)
{
    cbuf_t buf_out;
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

    int rc = send_headers_stream_verification(&buf_out, &h2s_idle);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_idle.current_stream.stream_id == 3, "stream_id of the new stream must be 3");
    TEST_ASSERT_MESSAGE(h2s_idle.last_open_stream_id == 3, "last open stream id must be 3");

    rc = send_headers_stream_verification(&buf_out, &h2s_parity);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_parity.current_stream.stream_id == 5, "stream_id of the new stream must be 5");
    TEST_ASSERT_MESSAGE(h2s_parity.last_open_stream_id == 5, "last open stream id must be 5");

    rc = send_headers_stream_verification(&buf_out, &h2s_id_gt);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.state == STREAM_OPEN, "stream state must be STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_id_gt.current_stream.stream_id == 27, "stream_id of the new stream must be 27");
    TEST_ASSERT_MESSAGE(h2s_id_gt.last_open_stream_id == 27, "last open stream id must be 27");

    rc = send_headers_stream_verification(&buf_out, &h2s_open);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_NO_ERROR, "rc must be HTTP2_RC_NO_ERROR");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.state == STREAM_OPEN, "stream state must remain STREAM_OPEN");
    TEST_ASSERT_MESSAGE(h2s_open.current_stream.stream_id == 23, "stream_id of the new stream must be 23");
    TEST_ASSERT_MESSAGE(h2s_open.last_open_stream_id == 23, "last open stream id must be 23");
}

void test_send_headers_stream_verification_errors(void)
{
    cbuf_t buf_out;
    h2states_t h2s_closed;
    h2states_t h2s_hcl;

    h2s_closed.current_stream.state = STREAM_CLOSED;
    h2s_hcl.current_stream.state = STREAM_HALF_CLOSED_LOCAL;

    frame_to_bytes_fake.return_val = 32;
    cbuf_push_fake.return_val = 32;

    int rc = send_headers_stream_verification(&buf_out, &h2s_closed);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream closed)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 1, "frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 1, "cbuf_push_fake must be called in send_goaway");

    rc = send_headers_stream_verification(&buf_out, &h2s_hcl);
    TEST_ASSERT_MESSAGE(rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT, "rc must be HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT (stream closed)");
    TEST_ASSERT_MESSAGE(frame_to_bytes_fake.call_count == 2, "frame to bytes must be called in send_goaway");
    TEST_ASSERT_MESSAGE(cbuf_push_fake.call_count == 2, "cbuf_push_fake must be called in send_goaway");

}

/*
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
    UNIT_TEST(test_change_stream_state_end_stream_flag_close_connection);
    UNIT_TEST(test_send_data);
    UNIT_TEST(test_send_data_end_stream);
    UNIT_TEST(test_send_data_full_sending);
    UNIT_TEST(test_send_data_errors);
    UNIT_TEST(test_send_data_close_connection);
    UNIT_TEST(test_send_settings_ack);
    UNIT_TEST(test_send_settings_ack_errors);
    UNIT_TEST(test_send_goaway);
    UNIT_TEST(test_send_goaway_close_connection);
    UNIT_TEST(test_send_goaway_errors);
    UNIT_TEST(test_send_window_update);
    UNIT_TEST(test_send_window_update_errors);
    UNIT_TEST(test_send_headers_stream_verification_server);
    UNIT_TEST(test_send_headers_stream_verification_client);
    UNIT_TEST(test_send_headers_stream_verification_errors);

    return UNIT_TESTS_END();
}
