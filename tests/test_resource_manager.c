#include "unit.h"

#include "fff.h"
#include "resource_manager.h"
#include "headers.h"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

#include <errno.h>

/*Import of functions not declared in http.h */
extern uint32_t get_data(http_data_t *data_in, uint8_t *data_buffer);
extern int set_data(http_data_t *data_out, uint8_t *data, int data_size);
extern int do_request(hstates_t *hs, char * method, char * uri);
extern int send_client_request(hstates_t *hs, char *method, char *uri, uint8_t *response, size_t *size);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, headers_clean, hstates_t *);
FAKE_VALUE_FUNC(int, headers_set, headers_t *, const char *, const char *);
FAKE_VALUE_FUNC(char *, headers_get, headers_t *, const char *);

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
    int gd = get_data(&data, data_size, buf, sizeof(buf));

    TEST_ASSERT_EQUAL( 4, gd);

    TEST_ASSERT_EQUAL( 0, memcmp(&buf, data, 4));
}

void test_set_data_success(void)
{
    uint8_t data[10];

    int sd = set_data(&dout, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL( 0, sd);

    TEST_ASSERT_EQUAL( 0, memcmp(&dout.buf, (uint8_t *)"test", 4));
}


void test_set_data_fail_zero_size(void)
{
    uint8_t data[10];

    int sd = set_data(&data, (uint8_t *)"", 0);

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
