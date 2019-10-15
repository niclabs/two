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

/*Import of functions not declared in http.h */
extern uint32_t get_data(uint8_t *data_in_buff, uint32_t data_in_buff_size, uint8_t *data_buffer, size_t size);
extern int set_data(uint8_t *data_buff, uint32_t *data_buff_size, uint8_t *data, int data_size);
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
    FAKE(headers_get)


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


/************************************
* TESTS
************************************/
void test_get_data_success(void)
{
    uint8_t data[10];

    memcpy(data, (uint8_t *)"test", 4);
    uint32_t data_size = 4;

    uint8_t buf[10];
    int gd = get_data((uint8_t *) &data, data_size,(uint8_t *) buf, sizeof(buf));

    TEST_ASSERT_EQUAL( 4, gd);

    TEST_ASSERT_EQUAL( 0, memcmp(&buf, data, 4));
}

void test_set_data_success(void)
{
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);

    int sd = set_data((uint8_t *) &data, &data_size, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL( 0, sd);

    TEST_ASSERT_EQUAL( 0, memcmp(&data, (uint8_t *)"test", 4));
    TEST_ASSERT_EQUAL( 4, data_size);
}


void test_set_data_fail_zero_size(void)
{
    uint8_t data[10];
    uint32_t data_size = (uint32_t)sizeof(data);

    int sd = set_data((uint8_t *) &data, &data_size, (uint8_t *)"", 0);

    TEST_ASSERT_EQUAL( -1, sd);
}



int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_get_data_success);

    UNIT_TEST(test_set_data_success);
    UNIT_TEST(test_set_data_fail_zero_size);

    return UNITY_END();
}
