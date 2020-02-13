#include "ll.h"
#include "unit.h"

struct li
{
    struct li *next;
    int num;
};

void setUp() {}

void test_list_operations(void)
{
    struct li first  = { .num = 10 };
    struct li second = { .num = 20 };
    struct li third  = { .num = 30 };

    struct li *list = NULL;

    TEST_ASSERT_EQUAL(0, LL_COUNT(list));

    LL_PUSH(&first, list);
    TEST_ASSERT_EQUAL(1, LL_COUNT(list));

    LL_APPEND(&second, list);
    TEST_ASSERT_EQUAL(2, LL_COUNT(list));

    struct li *found = LL_FIND(list, LL_ELEM(struct li)->num == 10);
    TEST_ASSERT_EQUAL(10, found->num);

    struct li *it = LL_POP(list);
    TEST_ASSERT_EQUAL(10, it->num);
    TEST_ASSERT_EQUAL(1, LL_COUNT(list));

    LL_PUSH(&third, list);
    TEST_ASSERT_EQUAL(2, LL_COUNT(list));

    it = LL_DELETE(&second, list);
    TEST_ASSERT_EQUAL(20, it->num);
    TEST_ASSERT_EQUAL(1, LL_COUNT(list));

    it = LL_POP(list);
    TEST_ASSERT_EQUAL(30, it->num);
    TEST_ASSERT_EQUAL(0, LL_COUNT(list));

    TEST_ASSERT_EQUAL(NULL, LL_POP(list));
    TEST_ASSERT_EQUAL(0, LL_COUNT(list));

    TEST_ASSERT_EQUAL(NULL, LL_DELETE(&second, list));
    TEST_ASSERT_EQUAL(0, LL_COUNT(list));
}

int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_list_operations);
    return UNITY_END();
}
