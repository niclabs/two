#include <string.h>

#include "unit.h"
#include "event.h"
#include "cbuf.h"


void setUp(void)
{}

void test_event_sock_create(void) {
    event_loop_t loop;
    event_loop_init(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS, event_sock_unused(&loop), "unused sockets should equal EVENT_MAX_SOCKETS after loop creation");
    for (int i = 0; i < EVENT_MAX_DESCRIPTORS; i++) {
        event_sock_create(&loop);
    }

    TEST_ASSERT_EQUAL_MESSAGE(0, event_sock_unused(&loop), "only EVENT_MAX_SOCKETS can be created with event_sock_create");
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_event_sock_create);
    UNIT_TESTS_END();
}
