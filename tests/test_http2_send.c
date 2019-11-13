#include "unit.h"
#include "logging.h"
#include "fff.h"
#include "http2/structs.h"
#include "http2/send.h"
#include "frames/common.h"
#include "cbuf.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(uint8_t, set_flag, uint8_t, uint8_t);

#define FFF_FAKES_LIST(FAKE)            \
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
