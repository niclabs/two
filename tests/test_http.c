#include "unit.h"

#include "fff.h"
#include "http.h"
#include "headers.h"
#include "resource_handler.h"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <stdint.h>

#include <errno.h>

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

/*Import of functions not declared in http.h */
extern uint32_t get_data(uint8_t *data_in_buff, uint32_t data_in_buff_size, uint8_t *data_buffer, size_t size);
extern int set_data(uint8_t *data_buff, uint32_t *data_buff_size, int max_data_buff_size, uint8_t *data, int data_size);
extern int do_request(uint8_t *data_buff, uint32_t *data_size, headers_t *headers_buff, char *method, char *uri);
extern int send_client_request(headers_t *headers_buff, char *method, char *uri, char *host);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, headers_clean, headers_t *);
FAKE_VALUE_FUNC(int, headers_set, headers_t *, const char *, const char *);
FAKE_VALUE_FUNC(char *, headers_get, headers_t *, const char *);
FAKE_VALUE_FUNC(http_resource_handler_t, resource_handler_get, char *, char *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)              \
    FAKE(headers_clean)                   \
    FAKE(headers_set)                     \
    FAKE(headers_get)                     \
    FAKE(resource_handler_get)


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

char header_name[10];
char header_value[10];
int headers_set_custom_fake(headers_t * headers, const char * head, const char * val)
{
    (void) headers;
    memcpy(header_name, head, strlen(head));
    memcpy(header_value, val, strlen(val));
    return 0;
}


int send_hello(char *method, char *uri, uint8_t *response, int maxlen)
{
    memcpy(response, "hello world!!!!", 16);
    return 16;
}

/************************************
* TESTS
************************************/

void test_get_data_success(void)
{
    // Create function parameters
    uint8_t data[10];
    memcpy(data, (uint8_t *)"test", 4);
    uint32_t data_size = 4;
    uint8_t buf[10];

    // Perform get
    int gd = get_data((uint8_t *) &data, data_size,(uint8_t *) buf, sizeof(buf));

    // Return value should be 4
    TEST_ASSERT_EQUAL( 4, gd);

    // Check if the data and buf have the same content
    TEST_ASSERT_EQUAL( 0, memcmp(&buf, data, 4));
}

void test_set_data_success(void)
{
    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);

    // Perform set
    int sd = set_data((uint8_t *) &data, &data_size, HTTP_MAX_RESPONSE_SIZE, (uint8_t *)"test", 4);

    // Return value should be 0
    TEST_ASSERT_EQUAL( 0, sd);

    // Check if the data and data_size have the correct content
    TEST_ASSERT_EQUAL( 0, memcmp(&data, (uint8_t *)"test", 4));
    TEST_ASSERT_EQUAL( 4, data_size);
}


void test_set_data_fail_zero_size(void)
{
    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);

    // Perform set
    int sd = set_data((uint8_t *) &data, &data_size, HTTP_MAX_RESPONSE_SIZE, (uint8_t *)"", 0);

    // Return value should be -1
    TEST_ASSERT_EQUAL( -1, sd);
}


void test_do_request_success(void)
{
    // Set auxiliary functions response
    resource_handler_get_fake.return_val = &send_hello;
    headers_set_fake.custom_fake = headers_set_custom_fake;

    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);
    headers_t headers_buff[1];

    // Perform request
    int res = do_request((uint8_t *) &data, &data_size, (headers_t *) &headers_buff, "GET", "/index");

    // Check that auxiliary functions was called only once
    TEST_ASSERT_EQUAL(1, resource_handler_get_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_clean_fake.call_count);

    // Check that headers_set was called with the correct parameters
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(":status", header_name);
    TEST_ASSERT_EQUAL_STRING("200", header_value);

    // Check if the data and data_size have the correct content
    uint32_t final_data_size = MIN((uint32_t)sizeof(data), (uint32_t) 16);
    TEST_ASSERT_EQUAL(final_data_size, data_size);
    TEST_ASSERT_EQUAL( 0, memcmp(&data, (uint8_t *)"hello world!!!!", 10));


    // Return value should be 0
    TEST_ASSERT_EQUAL(0, res);
}


void test_do_request_fail_resource_handler_get(void)
{
    // Set auxiliary functions response
    resource_handler_get_fake.return_val = NULL;
    headers_set_fake.custom_fake = headers_set_custom_fake;

    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);
    headers_t headers_buff[1];

    // Perform request
    int res = do_request((uint8_t *) &data, &data_size, (headers_t *) &headers_buff, "GET", "/index");

    // Check that auxiliary functions was called only once
    TEST_ASSERT_EQUAL(1, resource_handler_get_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_clean_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);

    // Check that headers_set was called with the correct parameters
    TEST_ASSERT_EQUAL_STRING(":status", header_name);
    TEST_ASSERT_EQUAL_STRING("404", header_value);

    // Check if the data and data_size have the correct content
    uint32_t final_data_size = MIN((uint32_t)sizeof(data), (uint32_t)strlen("Not Found"));
    TEST_ASSERT_EQUAL( final_data_size, data_size);
    TEST_ASSERT_EQUAL( 0, memcmp(&data, (uint8_t *)"Not Found", 9));


    // Return value should be 0
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "do_request should return 0 even if error response is sent");
}


void test_http_server_response_success(void)
{
    // Set auxiliary functions response
    char *returnVals[2] = { "GET", "/index" };
    SET_RETURN_SEQ(headers_get, returnVals, 2);
    resource_handler_get_fake.return_val = &send_hello;

    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);
    headers_t headers_buff[1];

    // Perform response
    int res = http_server_response((uint8_t *)&data, &data_size, (headers_t *)&headers_buff);

    // Check that the auxiliary function have been called twice
    TEST_ASSERT_EQUAL(2, headers_get_fake.call_count);

    // Return value should be 0
    TEST_ASSERT_EQUAL(0, res);
}


void test_http_server_response_fail_method_not_supported(void)
{
    // Set auxiliary functions response
    char *returnVals[2] = { "POST", "/index" };

    SET_RETURN_SEQ(headers_get, returnVals, 2);
    resource_handler_get_fake.return_val = &send_hello;
    headers_set_fake.custom_fake = headers_set_custom_fake;

    // Create function parameters
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);
    headers_t headers_buff[1];

    // Perform response
    int res = http_server_response((uint8_t *)&data, &data_size, (headers_t *)&headers_buff);

    // Check that the auxiliary function have been called twice
    TEST_ASSERT_EQUAL(2, headers_get_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);

    // Check that headers_set was called with the correct parameters
    TEST_ASSERT_EQUAL_STRING("501", header_value);

    // Check if the data and data_size have the correct content
    uint32_t final_data_size = MIN((uint32_t)sizeof(data), (uint32_t)strlen("Not Implemented"));
    TEST_ASSERT_EQUAL( final_data_size, data_size);
    TEST_ASSERT_EQUAL( 0, memcmp(&data, (uint8_t *)"Not Implemented", 10));

    // Return value should be 0
    TEST_ASSERT_EQUAL(0, res);
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_get_data_success);

    UNIT_TEST(test_set_data_success);
    UNIT_TEST(test_set_data_fail_zero_size);

    UNIT_TEST(test_do_request_success);
    UNIT_TEST(test_do_request_fail_resource_handler_get);

    UNIT_TEST(test_http_server_response_success);
    UNIT_TEST(test_http_server_response_fail_method_not_supported);

    return UNITY_END();
}
