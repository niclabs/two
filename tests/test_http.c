#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http.h"
#include "http_bridge.h"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

#include <errno.h>


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_create, sock_t *);
FAKE_VALUE_FUNC(int, sock_listen, sock_t *, uint16_t);
FAKE_VALUE_FUNC(int, sock_accept, sock_t *, sock_t *);
FAKE_VALUE_FUNC(int, sock_connect, sock_t *, char *, uint16_t);
FAKE_VALUE_FUNC(int, sock_read, sock_t *, char *, int, int);
FAKE_VALUE_FUNC(int, sock_write, sock_t *, char *, int);
FAKE_VALUE_FUNC(int, sock_destroy, sock_t *);
FAKE_VALUE_FUNC(int, h2_client_init_connection, hstates_t *);
FAKE_VALUE_FUNC(int, h2_server_init_connection, hstates_t *);
FAKE_VALUE_FUNC(int, h2_receive_frame, hstates_t *);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)            \
    FAKE(sock_create)                     \
    FAKE(sock_listen)                     \
    FAKE(sock_accept)                     \
    FAKE(sock_connect)                    \
    FAKE(sock_read)                       \
    FAKE(sock_write)                      \
    FAKE(sock_destroy)                    \
    FAKE(h2_client_init_connection)          \
    FAKE(h2_server_init_connection)          \
    FAKE(h2_receive_frame)          \


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
    return 4;
}

int foo(headers_lists_t * headers){
  (void)headers;
}


void test_http_init_server_success(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;
    sock_accept_fake.return_val = 0;
    h2_receive_frame_fake.return_val=0;
    sock_destroy_fake.return_val=0;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(h2_server_init_connection, returnVals, 2);

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[3]);
    TEST_ASSERT_EQUAL((void *)h2_receive_frame, fff.call_history[4]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[5]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[6]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[7]);

    TEST_ASSERT_EQUAL(-1, is);

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
}


void test_http_init_server_fail_h2_server_init_connection(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;
    sock_accept_fake.return_val = 0;
    h2_server_init_connection_fake.return_val = -1;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[3]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
}


void test_http_init_server_fail_sock_accept(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;
    sock_accept_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
}


void test_http_init_server_fail_sock_listen(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Partial error in server creation");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_init_server_fail_sock_create(void)
{
    hstates_t hs;
    sock_create_fake.return_val = -1;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Server could not be created");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_set_function_to_path_success(void){
    hstates_t hs;
    hs.path_callback_list_count=0;
    callback_type_t foo_callback;
    foo_callback.cb=foo;

    int set=http_set_function_to_path(&hs, foo_callback, "index");

    TEST_ASSERT_EQUAL(0,set);

    TEST_ASSERT_EQUAL(1, hs.path_callback_list_count);
    TEST_ASSERT_EQUAL(0, strncmp(hs.path_callback_list[0].name, "index", strlen("index")));
    TEST_ASSERT_EQUAL(foo_callback.cb, hs.path_callback_list[0].ptr);

}


void test_http_set_function_to_path_fail_list_full(void){
    hstates_t hs;
    hs.path_callback_list_count=HTTP_MAX_CALLBACK_LIST_ENTRY;
    callback_type_t foo_callback;

    int set=http_set_function_to_path(&hs, foo_callback, "index");

    TEST_ASSERT_EQUAL_MESSAGE(-1,set,"Path-callback list is full");
}


void test_http_set_header_success(void)
{
    hstates_t hs;
    int set = http_set_header(&hs, "settings", "server:on");

    TEST_ASSERT_EQUAL(0, set);
    TEST_ASSERT_EQUAL(1, hs.h_lists.header_list_count);
}


void test_http_set_header_fail_list_full(void)
{
    hstates_t hs;
    hs.h_lists.header_list_count=HTTP2_MAX_HEADER_COUNT;
    int set = http_set_header(&hs, "settings", "server:on");

    TEST_ASSERT_EQUAL_MESSAGE(-1, set,"Headers list is full");
    TEST_ASSERT_EQUAL(HTTP2_MAX_HEADER_COUNT, hs.h_lists.header_list_count);
}


void test_http_server_destroy_success(void)
{
    hstates_t hs;
    hs.socket_state = 1;
    hs.connection_state = 1;
    hs.server_socket_state = 1;

    sock_destroy_fake.return_val = 0;

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[1]);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_server_destroy_success_without_client(void)
{
    hstates_t hs;
    hs.socket_state = 0;
    hs.connection_state = 1;
    hs.server_socket_state = 1;

    sock_destroy_fake.return_val = 0;

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_server_destroy_fail_not_server(void)
{
    hstates_t hs;
    hs.server_socket_state = 0;

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Server not found");
}


void test_http_server_destroy_fail_sock_destroy(void)
{
    hstates_t hs;
    hs.socket_state = 1;
    hs.connection_state = 1;
    hs.server_socket_state = 1;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(sock_destroy, returnVals, 2);

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[1]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Error in server disconnection");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
}



void test_http_client_connect_success(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = 0;
    h2_client_init_connection_fake.return_val = 0;

    int cc = http_client_connect(&hs, 12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)h2_client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL(0, cc);

    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
}


void test_http_client_connect_fail_h2_client_init_connection(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = 0;
    h2_client_init_connection_fake.return_val = -1;

    int cc = http_client_connect(&hs, 12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)h2_client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Problems sending client data");

    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
}


void test_http_client_connect_fail_sock_connect(void)
{
    hstates_t hs;
    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int cc = http_client_connect(&hs, 12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client connection");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_client_connect_fail_sock_create(void)
{
    hstates_t hs;
    sock_create_fake.return_val = -1;

    int cc = http_client_connect(&hs, 12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client creation");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_get_header_success(void)
{
    hstates_t hs;

    http_set_header(&hs, "something", "something two");
    http_set_header(&hs, "settings", "server:on");

    char *buf = http_get_header(&hs, "settings");

    TEST_ASSERT_EQUAL(0, strncmp(buf, "server:on", strlen("server:on")));
}


void test_http_get_header_fail_empty_table(void)
{
    hstates_t hs;

    hs.h_lists.header_list_count = 0;
    char *buf = http_get_header(&hs, "settings");

    TEST_ASSERT_MESSAGE(NULL == buf, "Headers list is empty");
}


void test_http_get_header_fail_header_not_found(void)
{
    hstates_t hs;

    http_set_header(&hs, "something1", "something one");
    http_set_header(&hs, "something2", "something two");

    char *buf = http_get_header(&hs, "settings");

    TEST_ASSERT_MESSAGE(buf == NULL, "Header should not be found in headers list");
}


void test_http_client_disconnect_success_v1(void)
{
    hstates_t hs;
    hs.socket_state=0;

    int d = http_client_disconnect(&hs);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_client_disconnect_success_v2(void)
{
    hstates_t hs;
    hs.socket_state=1;

    sock_destroy_fake.return_val = 0;

    int d = http_client_disconnect(&hs);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_client_disconnect_fail(void)
{
    hstates_t hs;
    hs.socket_state=1;
    hs.connection_state=1;

    sock_destroy_fake.return_val = -1;

    int d = http_client_disconnect(&hs);


    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Client could not be disconnected");

    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_http_init_server_success);
    UNIT_TEST(test_http_init_server_fail_h2_server_init_connection);
    UNIT_TEST(test_http_init_server_fail_sock_accept);
    UNIT_TEST(test_http_init_server_fail_sock_listen);
    UNIT_TEST(test_http_init_server_fail_sock_create);

    UNIT_TEST(test_http_set_function_to_path_success);
    UNIT_TEST(test_http_set_function_to_path_fail_list_full);

    UNIT_TEST(test_http_set_header_success);
    UNIT_TEST(test_http_set_header_fail_list_full);

    UNIT_TEST(test_http_server_destroy_success);
    UNIT_TEST(test_http_server_destroy_success_without_client);
    UNIT_TEST(test_http_server_destroy_fail_not_server);
    UNIT_TEST(test_http_server_destroy_fail_sock_destroy);

    UNIT_TEST(test_http_client_connect_success);
    UNIT_TEST(test_http_client_connect_fail_h2_client_init_connection);
    UNIT_TEST(test_http_client_connect_fail_sock_connect);
    UNIT_TEST(test_http_client_connect_fail_sock_create);

    UNIT_TEST(test_http_get_header_success);
    UNIT_TEST(test_http_get_header_fail_empty_table);
    UNIT_TEST(test_http_get_header_fail_header_not_found);

    UNIT_TEST(test_http_client_disconnect_success_v1);
    UNIT_TEST(test_http_client_disconnect_success_v2);
    UNIT_TEST(test_http_client_disconnect_fail);

    return UNITY_END();
}
