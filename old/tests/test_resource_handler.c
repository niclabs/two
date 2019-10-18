#include "unit.h"

#include "fff.h"
#include "resource_handler.h"

#include <stdint.h>

#include <errno.h>

/*Import of functions not declared in resource_handler.h */
extern void resource_handler_reset(void);


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
* UTILS
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
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();
    resource_handler_set("GET", "/index", &send_hello);
    resource_handler_set("HEAD", "/index", &send_bye);

    // Perform get
    http_resource_handler_t get = resource_handler_get("GET", "/index");

    // Return value should be send_hello resource
    TEST_ASSERT_EQUAL(&send_hello, get);
}


void test_resource_handler_get_fail_no_resource_found(void)
{
    // Set context for the perform
    resource_handler_reset();

    // Perform get
    http_resource_handler_t get = resource_handler_get("HEAD", "/index");

    // Return value should be NULL
    TEST_ASSERT_EQUAL(NULL, get);
}


void test_resource_handler_set_success(void)
{
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();

    // Perform set
    int res = resource_handler_set("GET", "/index", &send_hello);

    // Return value should be 0
    TEST_ASSERT_EQUAL(0, res);

    // Check if, in fact, the resource was set
    http_resource_handler_t get = resource_handler_get("GET", "/index");
    TEST_ASSERT_EQUAL(&send_hello, get);
}


void test_resource_handler_set_success_replaced_resource(void)
{
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();
    resource_handler_set("GET", "/index", &send_hello);
    resource_handler_set("HEAD", "/index", &send_hello);
    resource_handler_set("GET", "/home", &send_hello);

    // Perform set
    int res = resource_handler_set("GET", "/index", &send_bye);

    // Return value should be 0
    TEST_ASSERT_EQUAL(0, res);

    // Check if, in fact, the resource was replaced
    http_resource_handler_t get = resource_handler_get("GET", "/index");
    TEST_ASSERT_EQUAL(&send_bye, get);
}


void test_resource_handler_set_fail_invalid_input(void)
{
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();
    resource_handler_set("GET", "/index", &send_hello);

    // Perform set
    int res = resource_handler_set("GET", "/index", NULL);

    // Return value should be -1
    TEST_ASSERT_EQUAL(-1, res);

    // Check if, in fact, the resource wasn't added
    http_resource_handler_t get = resource_handler_get("GET", "/index");
    TEST_ASSERT_EQUAL(&send_hello, get);
}


void test_resource_handler_set_fail_invalid_path(void)
{
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();

    // Perform set
    int res = resource_handler_set("GET", "index", &send_hello);

    // Return value should be -1
    TEST_ASSERT_EQUAL(-1, res);
}


void test_resource_handler_set_fail_null_path(void)
{
    // Set auxiliary functions response
    http_has_method_support_fake.return_val = 1;

    // Set context for the perform
    resource_handler_reset();

    // Perform set
    int res = resource_handler_set("GET", NULL, &send_hello);

    // Return value should be -1
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
