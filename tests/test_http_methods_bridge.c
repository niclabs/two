#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http_methods_bridge.h"
#include "http2.h"

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

#include <errno.h>


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_read, sock_t *, char *, int, int);
FAKE_VALUE_FUNC(int, sock_write, sock_t *, char *, int);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)            \
    FAKE(sock_read)                       \
    FAKE(sock_write)                      \


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


int sock_read_custom_fake(sock_t *s, char *buf, int l, int u)
{
    strcpy(buf, "hola");
    (void)s;
    (void)u;
    return 5;
}


void test_http_write_success(void)
{
    sock_write_fake.return_val = 5;

    uint8_t *buf = (uint8_t *)"hola";
    hstates_t hs;
    hs.socket_state = 1;
    int wr = http_write(&hs, buf, 256);


    TEST_ASSERT_EQUAL((void *)sock_write, fff.call_history[0]);

    TEST_ASSERT_EQUAL(5, wr);
}


void test_http_write_fail_sock_write(void)
{
    sock_write_fake.return_val = 0;

    uint8_t *buf = (uint8_t *)"hola";
    hstates_t hs;
    hs.socket_state = 1;
    int wr = http_write(&hs, buf, 256);


    TEST_ASSERT_EQUAL((void *)sock_write, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(0, wr, "Error in writing");
    TEST_ASSERT_EQUAL(0, hs.socket_state);

}


void test_http_write_fail_no_client_or_server(void)
{
    uint8_t *buf = (uint8_t *)"hola";
    hstates_t hs;

    hs.socket_state = 0;
    int wr = http_write(&hs, buf, 256);


    TEST_ASSERT_EQUAL_MESSAGE(-1, wr, "No client connected found");

}


void test_http_read_success(void)
{
    sock_read_fake.custom_fake = sock_read_custom_fake;

    uint8_t *buf = (uint8_t *)malloc(256);
    hstates_t hs;
    hs.socket_state = 1;
    int rd = http_read(&hs, buf, 256);


    TEST_ASSERT_EQUAL((void *)sock_read, fff.call_history[0]);

    TEST_ASSERT_EQUAL(0, strncmp((char *)buf, "hola", 5));

    TEST_ASSERT_EQUAL(5, rd);

    free(buf);
}


void test_http_read_fail_sock_read(void)
{
    sock_read_fake.return_val = 0;

    uint8_t *buf = (uint8_t *)malloc(256);
    hstates_t hs;
    hs.socket_state = 1;
    int rd = http_read(&hs, buf, 256);

    TEST_ASSERT_EQUAL((void *)sock_read, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(0, rd, "Error in reading");
    TEST_ASSERT_EQUAL(0, hs.socket_state);

    free(buf);
}



void test_http_read_fail_not_connected_client(void)
{
    uint8_t *buf = (uint8_t *)malloc(256);
    hstates_t hs;

    hs.socket_state = 0;
    int rd = http_read(&hs, buf, 256);

    TEST_ASSERT_EQUAL_MESSAGE(-1, rd, "No client connected found");

    free(buf);
}


void test_http_clear_header_list_success_new_index_bigger(void)
{
    hstates_t hs;

    hs.table_count = 4;
    int clear = http_clear_header_list(&hs, 5);

    TEST_ASSERT_EQUAL(0, clear);
    TEST_ASSERT_EQUAL(4,hs.table_count);
}


void test_http_clear_header_list_success_new_invalid_index(void)
{
    hstates_t hs;

    hs.table_count = 5;
    int clear = http_clear_header_list(&hs, -6);

    TEST_ASSERT_EQUAL(0, clear);
    TEST_ASSERT_EQUAL(0,hs.table_count);
}


void test_http_clear_header_list_success_new_valid_index(void)
{
    hstates_t hs;

    hs.table_count = 7;
    int clear = http_clear_header_list(&hs, 2);

    TEST_ASSERT_EQUAL(0, clear);
    TEST_ASSERT_EQUAL(2,hs.table_count);
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_http_write_success);
    UNIT_TEST(test_http_write_fail_sock_write);
    UNIT_TEST(test_http_write_fail_no_client_or_server);

    UNIT_TEST(test_http_read_success);
    UNIT_TEST(test_http_read_fail_sock_read);
    UNIT_TEST(test_http_read_fail_not_connected_client);

    UNIT_TEST(test_http_clear_header_list_success_new_index_bigger);
    UNIT_TEST(test_http_clear_header_list_success_new_invalid_index);
    UNIT_TEST(test_http_clear_header_list_success_new_valid_index);

    return UNITY_END();
}
