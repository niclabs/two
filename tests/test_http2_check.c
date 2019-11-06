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

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_check_incoming_headers_condition);

    return UNIT_TESTS_END();
}
