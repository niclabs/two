#include "unit.h"
#include "list.h"

struct li {
    struct li *next;
    int num;
};


void setUp()
{}


void test_list_operations(void)
{
    struct li first = { .num = 10 };
    struct li second = { .num = 20 };
    struct li third = { .num = 30 };

    struct li *list = NULL;

    TEST_ASSERT_EQUAL(0, LIST_COUNT(list));

    LIST_PUSH(&first, list);
    TEST_ASSERT_EQUAL(1, LIST_COUNT(list));

    LIST_APPEND(&second, list);
    TEST_ASSERT_EQUAL(2, LIST_COUNT(list));

    struct li *found = LIST_FIND(list, LIST_ELEM(struct li)->num == 10);
    TEST_ASSERT_EQUAL(10, found->num);

    struct li *it = LIST_POP(list);
    TEST_ASSERT_EQUAL(10, it->num);
    TEST_ASSERT_EQUAL(1, LIST_COUNT(list));

    LIST_PUSH(&third, list);
    TEST_ASSERT_EQUAL(2, LIST_COUNT(list));

    it = LIST_DELETE(&second, list);
    TEST_ASSERT_EQUAL(20, it->num);
    TEST_ASSERT_EQUAL(1, LIST_COUNT(list));

    it = LIST_POP(list);
    TEST_ASSERT_EQUAL(30, it->num);
    TEST_ASSERT_EQUAL(0, LIST_COUNT(list));

    TEST_ASSERT_EQUAL(NULL, LIST_POP(list));
    TEST_ASSERT_EQUAL(0, LIST_COUNT(list));

    TEST_ASSERT_EQUAL(NULL, LIST_DELETE(&second, list));
    TEST_ASSERT_EQUAL(0, LIST_COUNT(list));
}

int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_list_operations);
    return UNITY_END();
}
