#include "unity.h"
#include "sock.h"


void test_sock_create(void)
{
    TEST_ASSERT_EQUAL(1,1);
}

int main(void)
{
    UNITY_BEGIN();
    RUN_TEST(test_sock_create);
    return UNITY_END();
}
