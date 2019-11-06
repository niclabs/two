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
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_example);

    return UNIT_TESTS_END();
}
