#include "unit.h"

#include "fff.h"
#include "resource_handler.h"

#include <stdint.h>

#include <errno.h>

/*Import of functions not declared in resource_handler.h */
extern int is_valid_path(char *path);


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, http_has_method_support, char *);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)              \
    FAKE(http_has_method_support)


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

/************************************
* Utils
************************************/

int send_hello(char *method, char *uri, uint8_t *response, int maxlen)
{
    memcpy(response, "hello world!!!!", 16);
    return 16;
}

int send_bye(char *method, char *uri, uint8_t *response, int maxlen)
{
    memcpy(response, "bye bye!", 16);
    return 16;
}

/************************************
* TESTS
************************************/

void test_resource_handler_get_success(void)
{
    http_has_method_support_fake.return_val = 1;
    resource_handler_set("GET", "/index", &send_hello);

    http_resource_handler_t get = resource_handler_get("GET", "/index");

    TEST_ASSERT_EQUAL(&send_hello, get);
}


void test_resource_handler_get_fail_no_resource_found(void)
{
    http_resource_handler_t get = resource_handler_get("HEAD", "/index");

    TEST_ASSERT_EQUAL(NULL, get);
}


void test_resource_handler_set_success(void)
{
    http_has_method_support_fake.return_val = 1;

    int res = resource_handler_set("GET", "/index", &send_hello);

    TEST_ASSERT_EQUAL(0, res);

    http_resource_handler_t get = resource_handler_get("GET", "/index");

    TEST_ASSERT_EQUAL(&send_hello, get);
}


void test_resource_handler_set_success_replaced_resource(void)
{
    http_has_method_support_fake.return_val = 1;

    int res = resource_handler_set("GET", "/index", &send_bye);

    TEST_ASSERT_EQUAL(0, res);

    http_resource_handler_t get = resource_handler_get("GET", "/index");

    TEST_ASSERT_EQUAL(&send_bye, get);
}


void test_resource_handler_set_fail_invalid_input(void)
{
    http_has_method_support_fake.return_val = 1;

    int res = resource_handler_set("GET", "/index", NULL);

    TEST_ASSERT_EQUAL(-1, res);
}


void test_resource_handler_set_fail_invalid_path(void)
{
    http_has_method_support_fake.return_val = 1;

    int res = resource_handler_set("GET", "index", &send_hello);

    TEST_ASSERT_EQUAL(-1, res);
}


void test_resource_handler_set_fail_null_path(void)
{
    http_has_method_support_fake.return_val = 1;

    int res = resource_handler_set("GET", NULL, &send_hello);

    TEST_ASSERT_EQUAL(-1, res);
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_resource_handler_get_success);
    UNIT_TEST(test_resource_handler_get_fail_no_resource_found);

    UNIT_TEST(test_resource_handler_set_success);
    UNIT_TEST(test_resource_handler_set_success_replaced_resource);
    UNIT_TEST(test_resource_handler_set_fail_invalid_input);
    UNIT_TEST(test_resource_handler_set_fail_invalid_path);
    UNIT_TEST(test_resource_handler_set_fail_null_path);

    return UNITY_END();
}
