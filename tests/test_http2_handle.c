#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"              // for h2states_t
#include "http2/handle.h"
#include "frames/structs.h"             // for frame_header_t
#include "frames/data_frame.h"          // for data_payload_t
#include "frames/headers_frame.h"       // for headers_payload_t
#include "frames/settings_frame.h"      // for settings_payload_t
#include "frames/goaway_frame.h"        // for goaway_payload_t
#include "frames/ping_frame.h"          // for ping_payload_t
#include "frames/continuation_frame.h"  // for continuation_payload_t
#include "frames/window_update_frame.h" // for window_update_payload_t
#include "cbuf.h"                       // for cbuf

extern int update_settings_table(settings_payload_t *spl, uint8_t place, cbuf_t *buf_out, h2states_t *h2s);

#define LOCAL_SETTINGS 0
#define REMOTE_SETTINGS 1

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, flow_control_receive_data, h2states_t *, uint32_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, change_stream_state_end_stream_flag, uint8_t, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, http_server_response, uint8_t *, uint32_t *, header_list_t *);
FAKE_VALUE_FUNC(int, send_response, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, headers_validate, header_list_t *);
FAKE_VALUE_FUNC(char *, headers_get, header_list_t *, const char *);
FAKE_VALUE_FUNC(uint32_t, get_header_block_fragment_size, frame_header_t *, headers_payload_t *);
FAKE_VALUE_FUNC(int, receive_header_block, uint8_t *, int, header_list_t *, hpack_states_t *);
FAKE_VALUE_FUNC(uint32_t, headers_get_header_list_size, header_list_t *);
FAKE_VALUE_FUNC(uint32_t, read_setting_from, h2states_t *, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, send_goaway, uint32_t, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, send_ping, uint8_t *, int8_t, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, flow_control_receive_window_update, h2states_t *, uint32_t);
FAKE_VALUE_FUNC(uint32_t, update_window_size, h2_window_manager_t *, uint32_t );

#define FFF_FAKES_LIST(FAKE)                                    \
    FAKE(flow_control_receive_data)                             \
    FAKE(send_connection_error)                                 \
    FAKE(buffer_copy)                                           \
    FAKE(is_flag_set)                                           \
    FAKE(change_stream_state_end_stream_flag)                   \
    FAKE(http_server_response)                                  \
    FAKE(send_response)                                         \
    FAKE(headers_validate)                                      \
    FAKE(headers_get)                                           \
    FAKE(get_header_block_fragment_size)                        \
    FAKE(receive_header_block)                                  \
    FAKE(headers_get_header_list_size)                          \
    FAKE(read_setting_from)                                     \
    FAKE(send_goaway)                                           \
    FAKE(send_ping)                                             \
    FAKE(flow_control_receive_window_update)                    \
    FAKE(update_window_size)                                    \



void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);
    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_handle_data_payload_no_flags(void)
{
    frame_header_t head;

    head.length = 10;
    data_payload_t dpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.data.size = 0;
    // Fake settings
    flow_control_receive_data_fake.return_val = 0;
    buffer_copy_fake.return_val = 10;
    is_flag_set_fake.return_val = 0;
    int rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(rc, 0, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(h2s.data.size, head.length, "Data size must be equal to header payload's length");
}

void test_handle_data_payload_multi_data(void)
{
    frame_header_t head;

    head.length = 12;
    data_payload_t dpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.data.size = 0;
    // Fake settings
    flow_control_receive_data_fake.return_val = 0;
    int flag_set_returns[3] = { 0, 0, 1 };
    SET_RETURN_SEQ(is_flag_set, flag_set_returns, 3);
    http_server_response_fake.return_val = 0;
    send_response_fake.return_val = 0;
    headers_get_fake.return_val = "value";
    int rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(rc, 0, "Method should return 0. No errors were set");
    rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(rc, 0, "Method should return 0. No errors were set");
    rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(rc, 0, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(h2s.data.size, 3 * head.length, "Data size must be equal to 3 times payload's length");
}

void test_handle_data_payload_errors(void)
{
    frame_header_t head;

    head.length = 12;
    data_payload_t dpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.data.size = 0;
    h2s.received_end_stream = 1;
    // Fake settings
    int flow_rets[4] = { -1, 0, 0, 0 };
    SET_RETURN_SEQ(flow_control_receive_data, flow_rets, 4);
    int change_rets[3] = { -1, 0, 0 };
    SET_RETURN_SEQ(change_stream_state_end_stream_flag, change_rets, 3);
    char *headers_rets[4] = { NULL, "b", "a", "a" };
    SET_RETURN_SEQ(headers_get, headers_rets, 4);
    int flag_set_returns[3] = { 0, 0, 1 };
    SET_RETURN_SEQ(is_flag_set, flag_set_returns, 3);
    http_server_response_fake.return_val = -1;
    headers_get_fake.return_val = "value";
    int rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
    rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(2, rc, "Method should return 0. No errors were set");
    rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
    rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
}

void test_handle_headers_payload_no_flags(void)
{
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header_block_fragments_pointer = 0;
    // Set fake returns
    get_header_block_fragment_size_fake.return_val = 10;
    buffer_copy_fake.return_val = 10;
    is_flag_set_fake.return_val = 0;
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(10, h2s.header_block_fragments_pointer, "Pointer must be equal to 10");
}

void test_handle_headers_payload_end_stream_flag(void)
{
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header_block_fragments_pointer = 0;
    // Set fake returns
    get_header_block_fragment_size_fake.return_val = 10;
    buffer_copy_fake.return_val = 10;
    int flag_set_returns[2] = { 1, 0 };
    SET_RETURN_SEQ(is_flag_set, flag_set_returns, 2);
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(10, h2s.header_block_fragments_pointer, "Pointer must be equal to 10");
    TEST_ASSERT_EQUAL_MESSAGE(1, h2s.received_end_stream, "Received end stream must be equal to 1");
}

void test_handle_headers_payload_end_headers_flag(void)
{
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header_block_fragments_pointer = 0;
    h2s.received_end_stream = 0;
    // Set fake returns
    get_header_block_fragment_size_fake.return_val = 10;
    buffer_copy_fake.return_val = 10;
    int flag_set_returns[2] = { 0, 1 };
    SET_RETURN_SEQ(is_flag_set, flag_set_returns, 2);
    receive_header_block_fake.return_val = 10;
    headers_get_header_list_size_fake.return_val = 10;
    read_setting_from_fake.return_val = 20;
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.header_block_fragments_pointer, "Pointer must be equal to 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.received_end_stream, "Received end stream must be 0");
}

void test_handle_headers_payload_end_stream_and_headers(void)
{
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header_block_fragments_pointer = 0;
    h2s.received_end_stream = 0;
    // Set fake returns
    get_header_block_fragment_size_fake.return_val = 10;
    buffer_copy_fake.return_val = 10;
    int flag_set_returns[2] = { 1, 1 };
    SET_RETURN_SEQ(is_flag_set, flag_set_returns, 2);
    receive_header_block_fake.return_val = 10;
    change_stream_state_end_stream_flag_fake.return_val = 0;
    headers_get_fake.return_val = (char *)'a';
    http_server_response_fake.return_val = 0;
    send_response_fake.return_val = 0;
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.header_block_fragments_pointer, "Pointer must be equal to 0");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.received_end_stream, "Received end stream must be 0");
}

void test_handle_headers_payload_errors(void)
{
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header_block_fragments_pointer = 0;

    uint32_t get_header_block_rets[8] = { HTTP2_MAX_HBF_BUFFER + 1, 10, 10, 10, 10, 10, 10, 10 };
    SET_RETURN_SEQ(get_header_block_fragment_size, get_header_block_rets, 8);
    int buff_copy_rets[7] = { -1, 10, 10, 10, 10, 10, 10 };
    SET_RETURN_SEQ(buffer_copy, buff_copy_rets, 7);
    is_flag_set_fake.return_val = 1;
    int receive_header_rets[6] = { -1, -2, 100, 10, 10, 10 };
    SET_RETURN_SEQ(receive_header_block, receive_header_rets, 6);
    int change_stream_rets[3] = { -1, 0, 0 };
    SET_RETURN_SEQ(change_stream_state_end_stream_flag, change_stream_rets, 3)
    char *headers_rets[2] = { NULL, "b" };
    SET_RETURN_SEQ(headers_get, headers_rets, 2);
    http_server_response_fake.return_val = -1;
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(2, rc, "Return code must be 22 (HTTP2_RC_CLOSE_CONNECTION)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
    rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2 (HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT)");
}

void test_update_settings_table(void)
{
    settings_pair_t pairs[4];

    pairs[0].identifier = 0x2;
    pairs[0].value = 0;
    pairs[1].identifier = 0x4;
    pairs[1].value = 1024;
    pairs[2].identifier = 0x5;
    pairs[2].value = 18432;
    pairs[3].identifier = 0x6;
    pairs[3].value = 100;
    settings_payload_t spl;
    spl.pairs = pairs;
    spl.count = 4;
    cbuf_t bout;
    h2states_t h2s;
    h2s.remote_settings[0] = h2s.local_settings[0] = 1;
    h2s.remote_settings[1] = h2s.local_settings[1] = 1;
    h2s.remote_settings[2] = h2s.local_settings[2] = 1;
    h2s.remote_settings[3] = h2s.local_settings[3] = 1;
    h2s.remote_settings[4] = h2s.local_settings[4] = 1;
    h2s.remote_settings[5] = h2s.local_settings[5] = 1;
    int rc = update_settings_table(&spl, LOCAL_SETTINGS, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.local_settings[1], "Setting value was set to 0");
    TEST_ASSERT_EQUAL_MESSAGE(1024, h2s.local_settings[3], "Setting value was set to 1024");
    TEST_ASSERT_EQUAL_MESSAGE(18432, h2s.local_settings[4], "Setting value was set to 18432");
    TEST_ASSERT_EQUAL_MESSAGE(100, h2s.local_settings[5], "Setting value was set to 100");
}

void test_update_settings_table_errors(void)
{
    settings_pair_t pairs1[1];
    settings_pair_t pairs2[1];
    settings_pair_t pairs3[1];

    pairs1[0].identifier = 0x2;
    pairs1[0].value = 2;
    pairs2[0].identifier = 0x4;
    pairs2[0].value = 2147483648;
    pairs3[0].identifier = 0x5;
    pairs3[0].value = 16000;
    settings_payload_t spl1;
    spl1.pairs = pairs1;
    spl1.count = 1;
    settings_payload_t spl2;
    spl2.pairs = pairs2;
    spl2.count = 1;
    settings_payload_t spl3;
    spl3.pairs = pairs3;
    spl3.count = 1;
    cbuf_t bout;
    h2states_t h2s;
    h2s.remote_settings[0] = h2s.local_settings[0] = 1;
    h2s.remote_settings[1] = h2s.local_settings[1] = 1;
    h2s.remote_settings[2] = h2s.local_settings[2] = 1;
    h2s.remote_settings[3] = h2s.local_settings[3] = 1;
    h2s.remote_settings[4] = h2s.local_settings[4] = 1;
    h2s.remote_settings[5] = h2s.local_settings[5] = 1;
    int rc = update_settings_table(&spl1, LOCAL_SETTINGS, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
    rc = update_settings_table(&spl2, LOCAL_SETTINGS, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
    rc = update_settings_table(&spl3, LOCAL_SETTINGS, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Method should return 0. No errors were set");
}

void test_handle_goaway_payload_error_received(void)
{
    goaway_payload_t gapl;

    gapl.last_stream_id = 15;
    gapl.error_code = 0x1; // Error code
    cbuf_t bout;
    h2states_t h2s;
    int rc = handle_goaway_payload(&gapl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(2, rc, "Return code must be 2 (HTTP2_RC_CLOSE_CONNECTION)");
}

void test_handle_goaway_payload_no_error_stream_smaller(void)
{
    goaway_payload_t gapl;

    gapl.last_stream_id = 15;
    gapl.error_code = 0x0; // No error code
    cbuf_t bout;
    h2states_t h2s;
    h2s.current_stream.stream_id = 12;
    h2s.current_stream.state = 2;
    send_goaway_fake.return_val = 0;
    int rc = handle_goaway_payload(&gapl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_EQUAL_MESSAGE(2, h2s.current_stream.state, "Current stream must be closed.");
    TEST_ASSERT_EQUAL_MESSAGE(1, h2s.received_goaway, "GOAWAY was received.");
}

void test_handle_goaway_payload_no_error_stream_bigger(void)
{
    goaway_payload_t gapl;

    gapl.last_stream_id = 15;
    gapl.error_code = 0x0; // No error code
    cbuf_t bout;
    h2states_t h2s;
    h2s.current_stream.stream_id = 16;
    h2s.current_stream.state = 2;
    send_goaway_fake.return_val = 0;
    int rc = handle_goaway_payload(&gapl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_EQUAL_MESSAGE(4, h2s.current_stream.state, "Current stream must be closed.");
    TEST_ASSERT_EQUAL_MESSAGE(1, h2s.received_goaway, "GOAWAY was received.");
}

void test_handle_ping_payload_ack(void)
{
    ping_payload_t ppl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header.flags = 0x1;
    int rc = handle_ping_payload(&ppl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
}

void test_handle_ping_payload_send(void)
{
    ping_payload_t ppl;
    cbuf_t bout;
    h2states_t h2s;

    h2s.header.flags = 0x0;
    send_ping_fake.return_val = 0;
    int rc = handle_ping_payload(&ppl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_MESSAGE(send_ping_fake.call_count == 1, "Send ping must be called once");
}

void test_handle_continuation_payload_no_flags(void)
{
    frame_header_t header;

    header.length = 10;
    continuation_payload_t contpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.header_block_fragments_pointer = 5;
    buffer_copy_fake.return_val = 10;
    is_flag_set_fake.return_val = 0;
    int rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
}

void test_handle_continuation_payload_end_headers(void)
{
    frame_header_t header;

    header.length = 10;
    continuation_payload_t contpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.header_block_fragments_pointer = 5;
    h2s.received_end_stream = 0;
    buffer_copy_fake.return_val = 10;
    is_flag_set_fake.return_val = 1;
    receive_header_block_fake.return_val = 15;
    headers_get_header_list_size_fake.return_val = 15;
    read_setting_from_fake.return_val = 30;
    int rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.header_block_fragments_pointer, "Pointer must be 0, headers were read");
}

void test_handle_continuation_payload_end_headers_end_stream(void)
{
    frame_header_t header;

    header.length = 10;
    continuation_payload_t contpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.header_block_fragments_pointer = 5;
    h2s.received_end_stream = 1;
    buffer_copy_fake.return_val = 10;
    is_flag_set_fake.return_val = 1;
    receive_header_block_fake.return_val = 15;
    change_stream_state_end_stream_flag_fake.return_val = 0;
    headers_get_fake.return_val = (char *)'a';
    http_server_response_fake.return_val = 0;
    send_response_fake.return_val = 0;
    int rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_EQUAL_MESSAGE(0, h2s.header_block_fragments_pointer, "Pointer must be 0, headers were read");
}

void test_handle_continuation_errors(void)
{
    frame_header_t header;
    header.length = 20;
    frame_header_t big_header; // First error, header length
    big_header.length = HTTP2_MAX_HBF_BUFFER;
    continuation_payload_t contpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.header_block_fragments_pointer = 10;
    int buffer_rets[7] = {-1, 20, 20, 20, 20, 20};
    SET_RETURN_SEQ(buffer_copy, buffer_rets, 6);
    is_flag_set_fake.return_val = 1;
    int receive_rets[6] = {-1, -2, 20, 30, 30, 30};
    SET_RETURN_SEQ(receive_header_block, receive_rets, 6);
    int change_rets[3] = {-1, 0, 0};
    SET_RETURN_SEQ(change_stream_state_end_stream_flag, change_rets, 3);
    char *headers_rets[2] = { NULL, "b" };
    SET_RETURN_SEQ(headers_get, headers_rets, 2);
    http_server_response_fake.return_val = -1;
    int rc = handle_continuation_payload(&big_header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Header length error was forced");
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Buffer Copy error forced");
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Receive header block compression error forced");
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Receive header block internal error forced");
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Header block pointer is not equal to fragments received");
    h2s.header_block_fragments_pointer = 10;
    h2s.received_end_stream = 1;
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(2, rc, "Return code must be 2. Close connection was forced");
    h2s.header_block_fragments_pointer = 10;
    h2s.received_end_stream = 1;
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. Validate pseudo headers error was forced");
    h2s.header_block_fragments_pointer = 10;
    h2s.received_end_stream = 1;
    rc = handle_continuation_payload(&header, &contpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(-2, rc, "Return code must be -2. http server response error forced");
}

void test_handle_window_update_payload(void)
{
    window_update_payload_t wupl;

    wupl.window_size_increment = 20;
    cbuf_t bout;
    h2states_t h2s;
    flow_control_receive_window_update_fake.return_val = 0;
    int rc = handle_window_update_payload(&wupl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Return code must be 0 (HTTP2_RC_NO_ERROR)");
    TEST_ASSERT_EQUAL_MESSAGE(1, flow_control_receive_window_update_fake.call_count, "Flow control must be called once");
}
int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_handle_data_payload_no_flags);
    UNIT_TEST(test_handle_data_payload_multi_data);
    UNIT_TEST(test_handle_data_payload_errors);
    UNIT_TEST(test_handle_headers_payload_no_flags);
    UNIT_TEST(test_handle_headers_payload_end_stream_flag);
    UNIT_TEST(test_handle_headers_payload_end_headers_flag);
    UNIT_TEST(test_handle_headers_payload_end_stream_and_headers);
    UNIT_TEST(test_handle_headers_payload_errors);
    UNIT_TEST(test_update_settings_table);
    UNIT_TEST(test_update_settings_table_errors);
    UNIT_TEST(test_handle_goaway_payload_error_received);
    UNIT_TEST(test_handle_goaway_payload_no_error_stream_smaller);
    UNIT_TEST(test_handle_goaway_payload_no_error_stream_bigger);
    UNIT_TEST(test_handle_ping_payload_ack);
    UNIT_TEST(test_handle_ping_payload_send);
    UNIT_TEST(test_handle_continuation_payload_no_flags);
    UNIT_TEST(test_handle_continuation_payload_end_headers);
    UNIT_TEST(test_handle_continuation_payload_end_headers_end_stream);
    UNIT_TEST(test_handle_continuation_errors);
    UNIT_TEST(test_handle_window_update_payload);
    return UNIT_TESTS_END();
}
