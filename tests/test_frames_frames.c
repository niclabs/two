#include "unit.h"
#include "logging.h"

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
