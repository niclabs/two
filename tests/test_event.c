#include <string.h>

#include "unit.h"
#include "event.h"
#include "cbuf.h"


void setUp(void)
{}

void test_event_sock_create(void) {
    event_loop_t loop;
    event_loop_init(&loop);

    for (int i = 0; i < EVENT_MAX_DESCRIPTORS; i++) {
        event_sock_create(&loop);
    }

    TEST_ASSERT_EQUAL_MESSAGE(loop.unused, NULL, "at most EVENT_MAX_DESCRIPTORS sockets should be available for event");
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_event_sock_create);
    UNIT_TESTS_END();
}
