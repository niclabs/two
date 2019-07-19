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
extern int do_request(hstates_t *hs, char * method);
extern void reset_http_states(hstates_t *hs);
extern int http_set_header(headers_data_lists_t *hd_lists, char *name, char *value);
extern char *http_get_header(headers_data_lists_t *hd_lists, char *header, int header_size);
extern int http_set_data(headers_data_lists_t *hd_lists, uint8_t *data, int data_size);
extern uint32_t http_get_data(headers_data_lists_t *hd_lists, uint8_t *data_buffer);

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

int resource_handler(char * method, char * uri, uint8_t * response, int maxlen) {
    memcpy(response, "hello world!!!!", 16);
    return 16;
}

void test_reset_http_states_success(void)
{
    hstates_t hs;

    hs.socket_state = 1;
    hs.hd_lists.header_list_count_in = 1;
    hs.hd_lists.header_list_count_out = 1;
    hs.hd_lists.data_in_size = 1;
    hs.hd_lists.data_out_size = 1;
    hs.connection_state = 1;
    hs.server_socket_state = 1;
    hs.keep_receiving = 1;
    hs.new_headers = 1;

    reset_http_states(&hs);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_in_size);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_out_size);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.new_headers);
}


void test_http_server_create_success(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;

    int is = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);

    TEST_ASSERT_EQUAL(0, is);

    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_create_fail_sock_listen(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int is = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Partial error in server creation");

    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_create_fail_sock_create(void)
{
    hstates_t hs;

    sock_create_fake.return_val = -1;

    int is = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Error in server creation");

    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_start_success(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.server_socket_state = 1;
    hs.is_server = 1;

    sock_accept_fake.return_val = 0;
    sock_destroy_fake.return_val = -1;

    h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(h2_server_init_connection, returnVals, 2);

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)h2_receive_frame, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[4]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[5]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(1, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.new_headers);
}


void test_http_server_start_fail_h2_server_init_connection(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.server_socket_state = 1;

    sock_accept_fake.return_val = 0;
    h2_server_init_connection_fake.return_val = -1;

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)h2_server_init_connection, fff.call_history[1]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
}


void test_http_server_start_fail_sock_accept(void)
{
    hstates_t hs;

    reset_http_states(&hs);

    sock_accept_fake.return_val = -1;

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");

    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
}


void test_http_server_destroy_success(void)
{
    hstates_t hs;

    hs.socket_state = 1;
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

    hs.server_socket_state = 1;
    hs.socket_state = 1;

    int returnVals[2] = { -1, 0 };
    SET_RETURN_SEQ(sock_destroy, returnVals, 2);

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[1]);

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


void test_http_register_resource_success(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    int res = http_server_register_resource(&hs, "GET", "/index", &resource_handler);
    TEST_ASSERT_EQUAL(0, res);

    TEST_ASSERT_EQUAL(1, hs.resource_list_size);
    TEST_ASSERT_EQUAL_STRING("GET", hs.resource_list[0].method);
    TEST_ASSERT_EQUAL_STRING("/index", hs.resource_list[0].path);
    TEST_ASSERT_EQUAL(&resource_handler, hs.resource_list[0].handler);
}


void test_http_register_resource_fail_list_full(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    hs.resource_list_size = HTTP_MAX_RESOURCES;
    int res = http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "Register resource should return an error if resource list is full");
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
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_out);
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
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_out);
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
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_out);
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
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_in);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
}

/*
void test_http_get_success()
{
    hstates_t hs;
    response_received_type_t rr;

    reset_http_states(&hs);
    hs.connection_state = 1;
    hs.keep_receiving = 0;
    hs.new_headers = 1;

    h2_send_request_fake.return_val = 0;
    h2_receive_frame_fake.return_val = 0;

    int hg = http_get(&hs, "index", "example.org", "text", &rr);

    TEST_ASSERT_EQUAL(0, hg);

    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].name, ":path", strlen(":path")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].value, "index", strlen("index")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].name, "host", strlen("host")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].value, "example.org", strlen("example.org")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].name, "accept", strlen("accept")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].value, "text", strlen("text")));
}


void test_http_get_fail()
{
    hstates_t hs;
    response_received_type_t rr;

    reset_http_states(&hs);
    hs.connection_state = 1;
    hs.keep_receiving = 0;
    hs.new_headers = 1;

    h2_send_request_fake.return_val = 0;
    h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake;

    int hg = http_get(&hs, "index", "example.org", "text", &rr);

    TEST_ASSERT_EQUAL(-1, hg);

    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].name, ":path", strlen(":path")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].value, "index", strlen("index")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].name, "host", strlen("host")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].value, "example.org", strlen("example.org")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].name, "accept", strlen("accept")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].value, "text", strlen("text")));
}


void test_http_get_fail_h2_send_request()
{
    hstates_t hs;
    response_received_type_t rr;

    reset_http_states(&hs);

    h2_send_request_fake.return_val = -1;

    int hg = http_get(&hs, "index", "example.org", "text", &rr);

    TEST_ASSERT_EQUAL(-1, hg);

    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].name, ":path", strlen(":path")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[2].value, "index", strlen("index")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].name, "host", strlen("host")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[3].value, "example.org", strlen("example.org")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].name, "accept", strlen("accept")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[4].value, "text", strlen("text")));
}


void test_http_get_fail_headers_list_full()
{
    hstates_t hs;
    response_received_type_t rr;

    reset_http_states(&hs);
    hs.hd_lists.header_list_count_in = (uint8_t)256;
    hs.hd_lists.header_list_count_out = (uint8_t)256;

    h2_send_request_fake.return_val = -1;

    int hg = http_get(&hs, "index", "example.org", "text", &rr);

    TEST_ASSERT_EQUAL_MESSAGE(-1, hg, "Cannot send query");
}
*/

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

    hs.hd_lists.header_list_count_out = 0;
    int set = http_set_header(&hs.hd_lists, "settings", "server:on");

    TEST_ASSERT_EQUAL(0, set);
    TEST_ASSERT_EQUAL(1, hs.hd_lists.header_list_count_out);
}


void test_http_set_header_fail_list_full(void)
{
    hstates_t hs;

    hs.hd_lists.header_list_count_out = HTTP2_MAX_HEADER_COUNT;
    int set = http_set_header(&hs.hd_lists, "settings", "server:on");

    TEST_ASSERT_EQUAL_MESSAGE(-1, set, "Headers list is full");
    TEST_ASSERT_EQUAL(HTTP2_MAX_HEADER_COUNT, hs.hd_lists.header_list_count_out);
}


void test_http_get_header_success(void)
{
    hstates_t hs;

    strcpy(hs.hd_lists.header_list_in[0].name, "something1");
    strcpy(hs.hd_lists.header_list_in[0].value, "one");
    strcpy(hs.hd_lists.header_list_in[1].name, "settings");
    strcpy(hs.hd_lists.header_list_in[1].value, "server:on");

    hs.hd_lists.header_list_count_in = 2;

    char *buf = http_get_header(&hs.hd_lists, "settings", 8);

    TEST_ASSERT_EQUAL(0, strncmp(buf, "server:on", strlen("server:on")));
}


void test_http_get_header_fail_empty_table(void)
{
    hstates_t hs;

    hs.hd_lists.header_list_count_in = 0;
    char *buf = http_get_header(&hs.hd_lists, "settings", 8);

    TEST_ASSERT_MESSAGE(NULL == buf, "Headers list is empty");
}


void test_http_get_header_fail_header_not_found(void)
{
    hstates_t hs;

    strcpy(hs.hd_lists.header_list_in[0].name, "something1");
    strcpy(hs.hd_lists.header_list_in[0].value, "one");
    strcpy(hs.hd_lists.header_list_in[1].name, "something2");
    strcpy(hs.hd_lists.header_list_in[1].value, "two");

    hs.hd_lists.header_list_count_in = 2;

    char *buf = http_get_header(&hs.hd_lists, "settings", 8);

    TEST_ASSERT_MESSAGE(buf == NULL, "Header not found in headers list");
}


void test_http_set_data_success(void)
{
    headers_data_lists_t hd;

    int sd = http_set_data(&hd, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL( 0, sd);

    TEST_ASSERT_EQUAL( 4, hd.data_out_size);
    TEST_ASSERT_EQUAL( 0, memcmp(&hd.data_out, (uint8_t *)"test", 4));
}


void test_http_set_data_fail_big_data(void)
{
    headers_data_lists_t hd;

    hd.data_out_size = 0;

    int sd = http_set_data(&hd, (uint8_t *)"", 0);

    TEST_ASSERT_EQUAL_MESSAGE( -1, sd, "Data too large for buffer size");

    TEST_ASSERT_EQUAL( 0, hd.data_out_size);
}


void test_http_set_data_fail_data_full(void)
{
    headers_data_lists_t hd;

    hd.data_out_size = HTTP_MAX_DATA_SIZE;

    int sd = http_set_data(&hd, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL_MESSAGE( -1, sd, "Data buffer full");
}



void test_http_get_data_success(void){
    headers_data_lists_t hd;

    hd.data_in_size = 4;
    memcpy(hd.data_in, (uint8_t *)"test", 4);

    uint8_t buf[10];
    int gd = http_get_data(&hd, buf);

    TEST_ASSERT_EQUAL( 4, gd);

    TEST_ASSERT_EQUAL( 0, memcmp(&buf, hd.data_in, 4));
}


void test_http_get_data_fail_no_data(void){
    headers_data_lists_t hd;
    hd.data_in_size = 0;

    uint8_t buf[5];
    int gd = http_get_data( &hd, buf);

    TEST_ASSERT_EQUAL_MESSAGE( 0, gd, "Data list is empty");
}


void test_do_request_success(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    strcpy(hs.hd_lists.header_list_in[0].name, ":path");
    strcpy(hs.hd_lists.header_list_in[0].value, "/index");
    hs.hd_lists.header_list_count_in = 1;

    h2_send_response_fake.return_val = 0;

    int get = do_request(&hs, "GET");

    TEST_ASSERT_EQUAL((void *)h2_send_response, fff.call_history[0]);

    TEST_ASSERT_EQUAL(0, get);

    TEST_ASSERT_EQUAL(1, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL_STRING(":status", hs.hd_lists.header_list_out[0].name);
    TEST_ASSERT_EQUAL_STRING("200", hs.hd_lists.header_list_out[0].value);
}


void test_do_request_fail_h2_send_response(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    strcpy(hs.hd_lists.header_list_in[0].name, ":path");
    strcpy(hs.hd_lists.header_list_in[0].value, "/index");
    hs.hd_lists.header_list_count_in = 1;

    h2_send_response_fake.return_val = -1;

    int get = do_request(&hs, "GET");

    TEST_ASSERT_EQUAL((void *)h2_send_response, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, get, "Problems sending data");

    TEST_ASSERT_EQUAL(1, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL_STRING(":status", hs.hd_lists.header_list_out[0].name);
    TEST_ASSERT_EQUAL_STRING("200", hs.hd_lists.header_list_out[0].value);
}


void test_do_request_path_not_found(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    strcpy(hs.hd_lists.header_list_in[0].name, ":path");
    strcpy(hs.hd_lists.header_list_in[0].value, "/bla");
    hs.hd_lists.header_list_count_in = 1;

    int get = do_request(&hs, "GET");

    TEST_ASSERT_EQUAL_MESSAGE(0, get, "process_request should return 0 even if error response is sent");

    TEST_ASSERT_EQUAL(1, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL_STRING(":status", hs.hd_lists.header_list_out[0].name);
    TEST_ASSERT_EQUAL_STRING("404", hs.hd_lists.header_list_out[0].value);
}


void test_do_request_no_resources(void)
{
    hstates_t hs;

    reset_http_states(&hs);

    strcpy(hs.hd_lists.header_list_in[0].name, ":path");
    strcpy(hs.hd_lists.header_list_in[0].value, "/index");
    hs.hd_lists.header_list_count_in = 1;

    int get = do_request(&hs, "GET");

    TEST_ASSERT_EQUAL_MESSAGE(0, get, "Do request should return 0 even if no resources are found");

    TEST_ASSERT_EQUAL(1, hs.hd_lists.header_list_count_out);
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[0].name, ":status", strlen(":status")));
    TEST_ASSERT_EQUAL(0, strncmp(hs.hd_lists.header_list_out[0].value, "404", strlen("400")));
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_reset_http_states_success);

    UNIT_TEST(test_http_server_create_success);
    UNIT_TEST(test_http_server_create_fail_sock_listen);
    UNIT_TEST(test_http_server_create_fail_sock_create);

    UNIT_TEST(test_http_server_start_success);
    UNIT_TEST(test_http_server_start_fail_h2_server_init_connection);
    UNIT_TEST(test_http_server_start_fail_sock_accept);

    UNIT_TEST(test_http_server_destroy_success);
    UNIT_TEST(test_http_server_destroy_success_without_client);
    UNIT_TEST(test_http_server_destroy_fail_not_server);
    UNIT_TEST(test_http_server_destroy_fail_sock_destroy);

    UNIT_TEST(test_http_register_resource_success);
    UNIT_TEST(test_http_register_resource_fail_list_full);

    UNIT_TEST(test_http_client_connect_success);
    UNIT_TEST(test_http_client_connect_fail_h2_client_init_connection);
    UNIT_TEST(test_http_client_connect_fail_sock_connect);
    UNIT_TEST(test_http_client_connect_fail_sock_create);
/*
    UNIT_TEST(test_http_get_success);
    UNIT_TEST(test_http_get_fail);
    UNIT_TEST(test_http_get_fail_h2_send_request);
    UNIT_TEST(test_http_get_fail_headers_list_full);
*/
    UNIT_TEST(test_http_client_disconnect_success_v1);
    UNIT_TEST(test_http_client_disconnect_success_v2);
    UNIT_TEST(test_http_client_disconnect_fail);

    UNIT_TEST(test_http_set_header_success);
    UNIT_TEST(test_http_set_header_fail_list_full);

    UNIT_TEST(test_http_get_header_success);
    UNIT_TEST(test_http_get_header_fail_empty_table);
    UNIT_TEST(test_http_get_header_fail_header_not_found);

    UNIT_TEST(test_http_set_data_success);
    UNIT_TEST(test_http_set_data_fail_big_data);
    UNIT_TEST(test_http_set_data_fail_data_full);

    UNIT_TEST(test_http_get_data_success);
    UNIT_TEST(test_http_get_data_fail_no_data);

    UNIT_TEST(test_do_request_success);
    UNIT_TEST(test_do_request_fail_h2_send_response);
    UNIT_TEST(test_do_request_path_not_found);
    UNIT_TEST(test_do_request_no_resources);

    return UNITY_END();
}
