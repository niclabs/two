#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"  // for h2states_t
#include "http2/handle.h"
#include "frames/common.h"  // for frame_header_t
#include "frames/data_frame.h"  // for data_payload_t
#include "cbuf.h"           // for cbuf

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, flow_control_receive_data, h2states_t *, uint32_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);
FAKE_VOID_FUNC(buffer_copy, uint8_t*, uint8_t*, int);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, change_stream_state_end_stream_flag, uint8_t, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, http_server_response, uint8_t *, uint32_t *, headers_t *);
FAKE_VALUE_FUNC(int, send_response, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, headers_validate, headers_t*);
FAKE_VALUE_FUNC(char *, headers_get, headers_t *, const char *);

#define FFF_FAKES_LIST(FAKE)                                    \
    FAKE(flow_control_receive_data)                             \
    FAKE(send_connection_error)                                 \
    FAKE(buffer_copy)                                           \
    FAKE(is_flag_set)                                           \
    FAKE(change_stream_state_end_stream_flag)                   \
    FAKE(http_server_response)                                  \

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
    uint8_t nums[10];
    for(int i = 0; i < 10; i++){
      nums[i] = i+1;
    }
    dpl.data = nums;
    cbuf_t bout;
    h2states_t h2s;
    h2s.data.size = 0;
    // Fake settings
    flow_control_receive_data_fake.return_val = 0;
    is_flag_set_fake.return_val = 0;
    //cbuf_pop_fake.custom_fake = cbuf_pop_preface_fake;
    int rc = handle_data_payload(&head, &dpl, &bout, &h2s);
    TEST_ASSERT_EQUAL_MESSAGE(rc, 0, "Method should return 0. No errors were set");
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_handle_data_payload_no_flags);

    return UNIT_TESTS_END();
}
