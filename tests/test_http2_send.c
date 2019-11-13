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
FAKE_VALUE_FUNC(int, prepare_new_stream, h2states_t *);
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

void test_example(void)
{
    TEST_FAIL();
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_example);

    return UNIT_TESTS_END();
}
