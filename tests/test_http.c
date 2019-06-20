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

/*Import of functions not declared in http.h */
extern int get_receive(hstates_t *hs);
extern void set_init_values(hstates_t *hs);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_create, sock_t *);
FAKE_VALUE_FUNC(int, sock_listen, sock_t *, uint16_t);
FAKE_VALUE_FUNC(int, sock_accept, sock_t *, sock_t *);
FAKE_VALUE_FUNC(int, sock_connect, sock_t *, char *, uint16_t);
FAKE_VALUE_FUNC(int, sock_destroy, sock_t *);
FAKE_VALUE_FUNC(int, h2_client_init_connection, hstates_t *);
FAKE_VALUE_FUNC(int, h2_server_init_connection, hstates_t *);
FAKE_VALUE_FUNC(int, h2_receive_frame, hstates_t *);
FAKE_VALUE_FUNC(int, h2_send_response, hstates_t *);
FAKE_VALUE_FUNC(int, http_clear_header_list, hstates_t *, int, int);
FAKE_VALUE_FUNC(int, h2_send_request, hstates_t *);



/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)              \
    FAKE(sock_create)                     \
    FAKE(sock_listen)                     \
    FAKE(sock_accept)                     \
    FAKE(sock_connect)                    \
    FAKE(sock_destroy)                    \
    FAKE(h2_client_init_connection)       \
    FAKE(h2_server_init_connection)       \
    FAKE(h2_receive_frame)                \
    FAKE(h2_send_response)                \
    FAKE(http_clear_header_list)          \
    FAKE(h2_send_request)                 \


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


int h2_receive_frame_custom_fake(hstates_t *hs)
{
    hs->keep_receiving = 1;
    hs->connection_state = 0;
    return 1;
}

int foo(headers_lists_t *headers)
{
    http_set_header(headers, "ret", "ok");
    return 1;
}


void test_set_init_values_success(void)
{
    hstates_t hs;

    hs.socket_state = 1;
    hs.h_lists.header_list_count_in = 1;
    hs.h_lists.header_list_count_out = 1;
    hs.path_callback_list_count = 1;
    hs.connection_state = 1;
    hs.server_socket_state = 1;
    hs.keep_receiving = 1;
    hs.new_headers = 1;

    set_init_values(&hs);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.path_callback_list_count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.new_headers);
}


void test_http_init_server_success(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);

    TEST_ASSERT_EQUAL(0, is);

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
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

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_init_server_fail_sock_create(void)
{
    hstates_t hs;

    sock_create_fake.return_val = -1;

    int is = http_init_server(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Error in server creation");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_start_server_success(void)
{
    hstates_t hs;
    set_init_values(&hs);
    hs.server_socket_state = 1;

    sock_accept_fake.return_val = 0;
    sock_destroy_fake.return_val = 0;

    h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(h2_server_init_connection, returnVals, 2);

    int is = http_start_server(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)h2_receive_frame, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[4]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[5]);

    TEST_ASSERT_EQUAL(-1, is);

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
}


void test_http_start_server_fail_h2_server_init_connection(void)
{
    hstates_t hs;
    set_init_values(&hs);
    hs.server_socket_state = 1;

    sock_accept_fake.return_val = 0;
    h2_server_init_connection_fake.return_val = -1;

    int is = http_start_server(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[1]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
}


void test_http_start_server_fail_sock_accept(void)
{
    hstates_t hs;
    set_init_values(&hs);
    hs.server_socket_state = 1;

    sock_accept_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int is = http_start_server(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");

    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
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


void test_http_set_function_to_path_success(void)
{
    hstates_t hs;

    hs.path_callback_list_count = 0;
    callback_type_t foo_callback;
    foo_callback.cb = foo;

    int set = http_set_function_to_path(&hs, foo_callback, "index");

    TEST_ASSERT_EQUAL(0, set);

    TEST_ASSERT_EQUAL(1, hs.path_callback_list_count);
    TEST_ASSERT_EQUAL(0, strncmp(hs.path_callback_list[0].name, "index", strlen("index")));
    TEST_ASSERT_EQUAL(foo_callback.cb, hs.path_callback_list[0].ptr);

}


void test_http_set_function_to_path_fail_list_full(void)
{
    hstates_t hs;

    hs.path_callback_list_count = HTTP_MAX_CALLBACK_LIST_ENTRY;
    callback_type_t foo_callback;

    int set = http_set_function_to_path(&hs, foo_callback, "index");

    TEST_ASSERT_EQUAL_MESSAGE(-1, set, "Path-callback list is full");
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
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
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
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
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
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
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
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_client_disconnect_success_v1(void)
{
    hstates_t hs;

    hs.socket_state = 0;

    int d = http_client_disconnect(&hs);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
}


void test_http_client_disconnect_success_v2(void)
{
    hstates_t hs;

    hs.socket_state = 1;

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

    hs.socket_state = 1;
    hs.connection_state = 1;

    sock_destroy_fake.return_val = -1;

    int d = http_client_disconnect(&hs);


    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Client could not be disconnected");

    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
}


void test_http_set_header_success(void)
{
    hstates_t hs;
    int set = http_set_header(&hs.h_lists, "settings", "server:on");

    TEST_ASSERT_EQUAL(0, set);
    TEST_ASSERT_EQUAL(1, hs.h_lists.header_list_count_out);
}


void test_http_set_header_fail_list_full(void)
{
    hstates_t hs;

    hs.h_lists.header_list_count_out = HTTP2_MAX_HEADER_COUNT;
    int set = http_set_header(&hs.h_lists, "settings", "server:on");

    TEST_ASSERT_EQUAL_MESSAGE(-1, set, "Headers list is full");
    TEST_ASSERT_EQUAL(HTTP2_MAX_HEADER_COUNT, hs.h_lists.header_list_count_out);
}


void test_http_get_header_success(void)
{
    hstates_t hs;

    strcpy(hs.h_lists.header_list_in[0].name, "something1");
    strcpy(hs.h_lists.header_list_in[0].value, "one");
    strcpy(hs.h_lists.header_list_in[1].name, "settings");
    strcpy(hs.h_lists.header_list_in[1].value, "server:on");

    hs.h_lists.header_list_count_in = 2;

    char *buf = http_get_header(&hs.h_lists, "settings");

    TEST_ASSERT_EQUAL(0, strncmp(buf, "server:on", strlen("server:on")));
}


void test_http_get_header_fail_empty_table(void)
{
    hstates_t hs;

    hs.h_lists.header_list_count_in = 0;
    char *buf = http_get_header(&hs.h_lists, "settings");

    TEST_ASSERT_MESSAGE(NULL == buf, "Headers list is empty");
}


void test_http_get_header_fail_header_not_found(void)
{
    hstates_t hs;

    strcpy(hs.h_lists.header_list_in[0].name, "something1");
    strcpy(hs.h_lists.header_list_in[0].value, "one");
    strcpy(hs.h_lists.header_list_in[1].name, "something2");
    strcpy(hs.h_lists.header_list_in[1].value, "two");
    hs.h_lists.header_list_count_in = 2;

    char *buf = http_get_header(&hs.h_lists, "settings");

    TEST_ASSERT_MESSAGE(buf == NULL, "Header not found in headers list");
}


void test_get_receive_success(void)
{
    hstates_t hs;

    set_init_values(&hs);

    callback_type_t foo_callback;
    foo_callback.cb = foo;
    http_set_function_to_path(&hs, foo_callback, "index/");

    strcpy(hs.h_lists.header_list_in[0].name, ":path");
    strcpy(hs.h_lists.header_list_in[0].value, "index/");
    hs.h_lists.header_list_count_in = 1;

    h2_send_response_fake.return_val = 0;

    int get = get_receive(&hs);

    TEST_ASSERT_EQUAL((void *)h2_send_response, fff.call_history[0]);

    TEST_ASSERT_EQUAL(0, get);

    TEST_ASSERT_EQUAL(2, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].name, ":status", strlen(":status")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].value, "200", strlen("200")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[1].name, "ret", strlen("ret")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[1].value, "ok", strlen("ok")));
}


void test_get_receive_fail_h2_send_response(void)
{
    hstates_t hs;

    set_init_values(&hs);

    callback_type_t foo_callback;
    foo_callback.cb = foo;
    http_set_function_to_path(&hs, foo_callback, "index/");

    strcpy(hs.h_lists.header_list_in[0].name, ":path");
    strcpy(hs.h_lists.header_list_in[0].value, "index/");
    hs.h_lists.header_list_count_in = 1;

    h2_send_response_fake.return_val = -1;

    int get = get_receive(&hs);

    TEST_ASSERT_EQUAL((void *)h2_send_response, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, get, "Problems sending data");

    TEST_ASSERT_EQUAL(2, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].name, ":status", strlen(":status")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].value, "200", strlen("200")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[1].name, "ret", strlen("ret")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[1].value, "ok", strlen("ok")));
}


void test_get_receive_path_not_found(void)
{
    hstates_t hs;

    set_init_values(&hs);

    callback_type_t foo_callback;
    http_set_function_to_path(&hs, foo_callback, "index/out");

    strcpy(hs.h_lists.header_list_in[0].name, ":path");
    strcpy(hs.h_lists.header_list_in[0].value, "index/");
    hs.h_lists.header_list_count_in = 1;

    int get = get_receive(&hs);

    TEST_ASSERT_EQUAL_MESSAGE(0, get, "No function associated with this path");

    TEST_ASSERT_EQUAL(1, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].name, ":status", strlen(":status")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].value, "400", strlen("400")));
}


void test_get_receive_path_callback_list_empty(void)
{
    hstates_t hs;

    set_init_values(&hs);

    strcpy(hs.h_lists.header_list_in[0].name, ":path");
    strcpy(hs.h_lists.header_list_in[0].value, "index/");
    hs.h_lists.header_list_count_in = 1;

    int get = get_receive(&hs);

    TEST_ASSERT_EQUAL_MESSAGE(0, get, "Path-callback list is empty");

    TEST_ASSERT_EQUAL(1, hs.h_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].name, ":status", strlen(":status")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.h_lists.header_list_out[0].value, "400", strlen("400")));
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_set_init_values_success);

    UNIT_TEST(test_http_init_server_success);
    UNIT_TEST(test_http_init_server_fail_sock_listen);
    UNIT_TEST(test_http_init_server_fail_sock_create);

    UNIT_TEST(test_http_start_server_success);
    UNIT_TEST(test_http_start_server_fail_h2_server_init_connection);
    UNIT_TEST(test_http_start_server_fail_sock_accept);

    UNIT_TEST(test_http_server_destroy_success);
    UNIT_TEST(test_http_server_destroy_success_without_client);
    UNIT_TEST(test_http_server_destroy_fail_not_server);
    UNIT_TEST(test_http_server_destroy_fail_sock_destroy);

    UNIT_TEST(test_http_set_function_to_path_success);
    UNIT_TEST(test_http_set_function_to_path_fail_list_full);

    UNIT_TEST(test_http_client_connect_success);
    UNIT_TEST(test_http_client_connect_fail_h2_client_init_connection);
    UNIT_TEST(test_http_client_connect_fail_sock_connect);
    UNIT_TEST(test_http_client_connect_fail_sock_create);

    UNIT_TEST(test_http_client_disconnect_success_v1);
    UNIT_TEST(test_http_client_disconnect_success_v2);
    UNIT_TEST(test_http_client_disconnect_fail);

    UNIT_TEST(test_http_set_header_success);
    UNIT_TEST(test_http_set_header_fail_list_full);

    UNIT_TEST(test_http_get_header_success);
    UNIT_TEST(test_http_get_header_fail_empty_table);
    UNIT_TEST(test_http_get_header_fail_header_not_found);

    UNIT_TEST(test_get_receive_success);
    UNIT_TEST(test_get_receive_fail_h2_send_response);
    UNIT_TEST(test_get_receive_path_not_found);
    UNIT_TEST(test_get_receive_path_callback_list_empty);

    return UNITY_END();
}
