#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "http2/structs.h"  // for h2states_t
#include "http2/check.h"
#include "frames/common.h"  // for frame_header_t
#include "cbuf.h"           // for cbuf

// Include header definitions for file to test 
// e.g #include "sock.h"

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
