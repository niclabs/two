#include "content_type.h"
#include "unit.h"

void setUp(void) {}

void test_content_type(void)
{
    TEST_ASSERT_EQUAL_STRING("text/plain", content_type_allowed("text/plain"));
    TEST_ASSERT_EQUAL_STRING("application/json",
                             content_type_allowed("application/json"));
    TEST_ASSERT_EQUAL_STRING("application/cbor",
                             content_type_allowed("application/cbor"));

    TEST_ASSERT_EQUAL(NULL, content_type_allowed("application/binary"));
}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_content_type);

    return UNITY_END();
}
