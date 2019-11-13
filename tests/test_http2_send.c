#include "unit.h"
#include "logging.h"
#include "fff.h"
#include "http2/structs.h"
#include "http2/send.h"
#include "frames/common.h"
#include "cbuf.h"


void test_example(void) {
    TEST_FAIL();
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    
    // Call tests here
    UNIT_TEST(test_example);

    return UNIT_TESTS_END();
}
