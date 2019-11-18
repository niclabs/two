#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"          // for h2states_t
#include "http2/handle.h"
#include "frames/common.h"          // for frame_header_t
#include "frames/data_frame.h"      // for data_payload_t
#include "frames/headers_frame.h"   // for headers_payload_t
#include "cbuf.h"                   // for cbuf

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, flow_control_receive_data, h2states_t *, uint32_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, change_stream_state_end_stream_flag, uint8_t, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, http_server_response, uint8_t *, uint32_t *, headers_t *);
FAKE_VALUE_FUNC(int, send_response, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, headers_validate, headers_t *);
FAKE_VALUE_FUNC(char *, headers_get, headers_t *, const char *);
FAKE_VALUE_FUNC(uint32_t, get_header_block_fragment_size, frame_header_t *, headers_payload_t *);
FAKE_VALUE_FUNC(int, receive_header_block, uint8_t *, int, headers_t *, hpack_states_t *);
FAKE_VALUE_FUNC(uint32_t, headers_get_header_list_size, headers_t *);
FAKE_VALUE_FUNC(uint32_t, read_setting_from, h2states_t *, uint8_t, uint8_t);

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


void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);
    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_handle_data_payload_no_flags(void) {
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

void test_handle_data_payload_multi_data(void) {
    frame_header_t head;
    head.length = 12;
    data_payload_t dpl;
    cbuf_t bout;
    h2states_t h2s;
    h2s.data.size = 0;
    // Fake settings
    flow_control_receive_data_fake.return_val = 0;
    int flag_set_returns[3] = {0,0,1};
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
    TEST_ASSERT_EQUAL_MESSAGE(h2s.data.size, 3*head.length, "Data size must be equal to header payload's length");
}

void test_handle_headers_payload(void){
    frame_header_t head;
    headers_payload_t hpl;
    cbuf_t bout;
    h2states_t h2s;
    int rc = handle_headers_payload(&head, &hpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(0, rc, "Method should return 0. No errors were set");
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_handle_data_payload_no_flags);
    UNIT_TEST(test_handle_data_payload_multi_data);
    // UNIT_TEST(test_handle_headers_payload);
    return UNIT_TESTS_END();
}
