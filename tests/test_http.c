#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http.h"
#include "http_bridge.h"
#include "headers.h"


#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdint.h>

#include <errno.h>

/*Import of functions not declared in http.h */
extern int do_request(hstates_t *hs, char * method, char * uri);
extern void reset_http_states(hstates_t *hs);
extern int set_data(headers_data_lists_t *hd_lists, uint8_t *data, int data_size);
extern uint32_t get_data(headers_data_lists_t *hd_lists, uint8_t *data_buffer);
extern int receive_server_response(hstates_t *hs);
extern int send_client_request(hstates_t *hs, char *method, char *uri, uint8_t *response, size_t *size);

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
FAKE_VALUE_FUNC(int, headers_init, headers_t *, header_t *, int);
FAKE_VALUE_FUNC(int, headers_set, headers_t *, const char *, const char *);
FAKE_VALUE_FUNC(char *, headers_get, headers_t *, const char *);


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
    FAKE(h2_send_request)                 \
    FAKE(headers_init)                    \
    FAKE(headers_set)                     \
    FAKE(headers_get)


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}



int headers_init_custom_fake(headers_t *headers, header_t *hlist, int maxlen)
{
    memset(headers, 0, sizeof(*headers));
    memset(hlist, 0, maxlen * sizeof(*hlist));
    headers->headers = hlist;
    headers->maxlen = maxlen;
    headers->count = 0;
    return 0;
}

int h2_receive_frame_custom_fake(hstates_t *hs)
{
    if (h2_receive_frame_fake.call_count == 1) {
        hs->keep_receiving = 1;
    }
    if (h2_receive_frame_fake.call_count == 2) {
        hs->keep_receiving = 0;
        hs->hd_lists.data_in_size = 5;
    }
    return 0;
}


int h2_receive_frame_custom_fake_connection_state(hstates_t *hs)
{
    hs->connection_state = 0;
    return 1;
}

int resource_handler(char * method, char * uri, uint8_t * response, int maxlen) {
    memcpy(response, "hello world!!!!", 16);
    return 16;
}


/************************************
* TESTS
************************************/
void test_set_data_success(void)
{
    headers_data_lists_t hd;

    int sd = set_data(&hd, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL( 0, sd);

    TEST_ASSERT_EQUAL( 4, hd.data_out_size);
    TEST_ASSERT_EQUAL( 0, memcmp(&hd.data_out, (uint8_t *)"test", 4));
}


void test_set_data_fail_big_data(void)
{
    headers_data_lists_t hd;

    hd.data_out_size = 0;

    int sd = set_data(&hd, (uint8_t *)"", 0);

    TEST_ASSERT_EQUAL_MESSAGE( -1, sd, "Data too large for buffer size");

    TEST_ASSERT_EQUAL( 0, hd.data_out_size);
}


void test_set_data_fail_data_full(void)
{
    headers_data_lists_t hd;

    hd.data_out_size = HTTP_MAX_DATA_SIZE;

    int sd = set_data(&hd, (uint8_t *)"test", 4);

    TEST_ASSERT_EQUAL_MESSAGE( -1, sd, "Data buffer full");
}



void test_get_data_success(void){
    headers_data_lists_t hd;

    hd.data_in_size = 4;
    memcpy(hd.data_in, (uint8_t *)"test", 4);

    uint8_t buf[10];
    int gd = get_data(&hd, buf);

    TEST_ASSERT_EQUAL( 4, gd);

    TEST_ASSERT_EQUAL( 0, memcmp(&buf, hd.data_in, 4));
}


void test_get_data_fail_no_data(void){
    headers_data_lists_t hd;
    hd.data_in_size = 0;

    uint8_t buf[5];
    int gd = get_data( &hd, buf);

    TEST_ASSERT_EQUAL_MESSAGE( 0, gd, "Data list is empty");
}

void test_reset_http_states_success(void)
{
    hstates_t hs;

    hs.socket_state = 1;
    hs.headers_in.count = 1;
    hs.headers_out.count = 1;
    hs.hd_lists.data_in_size = 1;
    hs.hd_lists.data_out_size = 1;
    hs.connection_state = 1;
    hs.server_socket_state = 1;
    hs.keep_receiving = 1;
    hs.new_headers = 1;

    reset_http_states(&hs);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.headers_in.count);
    TEST_ASSERT_EQUAL(0, hs.headers_out.count);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_in_size);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_out_size);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.new_headers);
}


void test_do_request_success(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    // Register http resource
    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    // Set send response
    h2_send_response_fake.return_val = 0;

    // Perform request
    int res = do_request(&hs, "GET", "/index");

    // Check that headers_set was called with the correct parameters
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(":status", headers_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING("200", headers_set_fake.arg2_val);

    // Check that send response was called
    TEST_ASSERT_EQUAL(1, h2_send_response_fake.call_count);

    // Return value should be 1
    TEST_ASSERT_EQUAL(0, res);
}


void test_do_request_fail_h2_send_response(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    // Register http resource
    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    // Set error response from send response
    h2_send_response_fake.return_val = -1;

    // Perform request
    int res = do_request(&hs, "GET", "/index");

    // Check that headers_set was called with the correct parameters
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(":status", headers_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING("200", headers_set_fake.arg2_val);

    // Check that send response was called
    TEST_ASSERT_EQUAL(1, h2_send_response_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "h2_send_response error should trigger error response from do_request");
}


void test_do_request_path_not_found(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    // Register http resource
    http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    // Perform request
    int res = do_request(&hs, "GET", "/bla");

    // Check that headers_set was called with the correct parameters in error()
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(":status", headers_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING("404", headers_set_fake.arg2_val);

    // Test correct return value
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "do_request should return 0 even if error response is sent");
}


void test_do_request_no_resources(void)
{
    hstates_t hs;
    reset_http_states(&hs);

    // Register http resource
    int res = do_request(&hs, "GET", "/index");

    // Check that headers_set was called with the correct parameters in error()
    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(":status", headers_set_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING("404", headers_set_fake.arg2_val);

    // Test correct return value
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "do_request should return 0 even if no resources are found");
}


void test_http_server_create_success(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = 0;
    FFF_RESET_HISTORY();

    int sc = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL(sock_create_fake.call_count,1);
    TEST_ASSERT_EQUAL(sock_listen_fake.call_count, 1);

    TEST_ASSERT_EQUAL(0, sc);

    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_create_fail_sock_listen(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_listen_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int sc = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL(sock_create_fake.call_count, 1);
    TEST_ASSERT_EQUAL(sock_listen_fake.call_count, 1);
    TEST_ASSERT_EQUAL(sock_destroy_fake.call_count, 1);

    TEST_ASSERT_EQUAL_MESSAGE(-1, sc, "Partial error in server creation");

    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_create_fail_sock_create(void)
{
    hstates_t hs;

    sock_create_fake.return_val = -1;

    int sc = http_server_create(&hs, 12);

    TEST_ASSERT_EQUAL(sock_create_fake.call_count, 1);

    TEST_ASSERT_EQUAL_MESSAGE(-1, sc, "Error in server creation");

    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(1, hs.is_server);
}


void test_http_server_start_success(void)
{
    hstates_t hs;

    headers_init_fake.custom_fake = headers_init_custom_fake;
    reset_http_states(&hs);


    hs.server_socket_state = 1;
    hs.is_server = 1;

    sock_accept_fake.return_val = 0;
    sock_destroy_fake.return_val = -1;

    h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(h2_server_init_connection, returnVals, 2);

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL(2, sock_accept_fake.call_count);
    TEST_ASSERT_EQUAL(2, h2_server_init_connection_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_receive_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_destroy_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(1, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.new_headers);
}


void test_http_server_start_fail_h2_server_init_connection(void)
{
    hstates_t hs;
    headers_init_fake.custom_fake = headers_init_custom_fake;
    reset_http_states(&hs);
    hs.server_socket_state = 1;

    sock_accept_fake.return_val = 0;
    h2_server_init_connection_fake.return_val = -1;

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL(1, sock_accept_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_server_init_connection_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");

    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
}


void test_http_server_start_fail_sock_accept(void)
{
    hstates_t hs;
    headers_init_fake.custom_fake = headers_init_custom_fake;
    reset_http_states(&hs);

    sock_accept_fake.return_val = -1;

    int is = http_server_start(&hs);

    TEST_ASSERT_EQUAL(1, sock_accept_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");

    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.socket_state);
}


void test_http_server_destroy_success(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.socket_state = 1;
    hs.server_socket_state = 1;

    sock_destroy_fake.return_val = 0;

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL(2, sock_destroy_fake.call_count);

    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_server_destroy_fail_not_server(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.server_socket_state = 0;

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Server not found");
}


void test_http_server_destroy_success_without_client(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.server_socket_state = 1;
    hs.socket_state = 1;

    int returnVals[2] = { -1, 0 };
    SET_RETURN_SEQ(sock_destroy, returnVals, 2);

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL(2, sock_destroy_fake.call_count);


    TEST_ASSERT_EQUAL(0, d);

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_server_destroy_fail_sock_destroy(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.socket_state = 1;
    hs.server_socket_state = 1;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(sock_destroy, returnVals, 2);

    int d = http_server_destroy(&hs);

    TEST_ASSERT_EQUAL(2, sock_destroy_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Error in server disconnection");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(1, hs.server_socket_state);
}


void test_http_register_resource_success_v1(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = 0;

    int res = http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    TEST_ASSERT_EQUAL(0, res);

    TEST_ASSERT_EQUAL(1, hs.resource_list_size);
    TEST_ASSERT_EQUAL_STRING("GET", hs.resource_list[0].method);
    TEST_ASSERT_EQUAL_STRING("/index", hs.resource_list[0].path);
    TEST_ASSERT_EQUAL(&resource_handler, hs.resource_list[0].handler);
}


void test_http_register_resource_success_v2(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = 1;

    strncpy(hs.resource_list[0].method, "GET", 8);
    strncpy(hs.resource_list[0].path, "/index", HTTP_MAX_PATH_SIZE);

    int res = http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    TEST_ASSERT_EQUAL(0, res);

    TEST_ASSERT_EQUAL(1, hs.resource_list_size);
    TEST_ASSERT_EQUAL_STRING("GET", hs.resource_list[0].method);
    TEST_ASSERT_EQUAL_STRING("/index", hs.resource_list[0].path);
    TEST_ASSERT_EQUAL(&resource_handler, hs.resource_list[0].handler);
}


void test_http_register_resource_fail_invalid_input(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = 0;

    int res = http_server_register_resource(&hs, "GET", "/index", NULL);

    TEST_ASSERT_EQUAL(-1, res);
}


void test_http_register_resource_fail_not_supported_method(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = 0;

    int res = http_server_register_resource(&hs, "POST", "/index", &resource_handler);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "Method POST not implemented yet");
}


void test_http_register_resource_fail_invalid_path(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = 0;

    int res = http_server_register_resource(&hs, "GET", "index", &resource_handler);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "Path index does not have a valid format");
}

void test_http_register_resource_fail_list_full(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.resource_list_size = HTTP_MAX_RESOURCES;

    int res = http_server_register_resource(&hs, "GET", "/index", &resource_handler);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "Register resource should return an error if resource list is full");
}


void test_receive_server_response_success(void) {
  hstates_t hs;
  reset_http_states(&hs);
  hs.connection_state = 1;

  h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake;

  int rsr = receive_server_response(&hs);

  TEST_ASSERT_EQUAL(2, h2_receive_frame_fake.call_count);

  TEST_ASSERT_EQUAL(0, rsr);

  TEST_ASSERT_EQUAL(1, hs.connection_state);
  TEST_ASSERT_EQUAL(0, hs.keep_receiving);
  TEST_ASSERT_EQUAL(5, hs.hd_lists.data_in_size);
}


void test_receive_server_response_fail_h2_receive_frame(void)
{
    hstates_t hs;
    reset_http_states(&hs);
    hs.connection_state = 1;
    hs.hd_lists.data_in_size = 0;

    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(h2_receive_frame, returnVals, 2);

    int rsr = receive_server_response(&hs);

    TEST_ASSERT_EQUAL(2, h2_receive_frame_fake.call_count);

    TEST_ASSERT_EQUAL(-1, rsr);

    TEST_ASSERT_EQUAL(1, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_in_size);
}


void test_receive_server_response_fail_connection_state(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.connection_state = 1;
    hs.hd_lists.data_in_size = 0;

    h2_receive_frame_fake.custom_fake = h2_receive_frame_custom_fake_connection_state;

    int rsr = receive_server_response(&hs);

    TEST_ASSERT_EQUAL(1, h2_receive_frame_fake.call_count);

    TEST_ASSERT_EQUAL(-1, rsr);

    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.keep_receiving);
    TEST_ASSERT_EQUAL(0, hs.hd_lists.data_in_size);
}


void test_send_client_request_success(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.connection_state = 1;
    hs.hd_lists.data_in_size = 8;
    hs.keep_receiving = 0;
    uint8_t *buf = (uint8_t *)"hola";
    memcpy(hs.hd_lists.data_in, buf, 8);

    headers_init_fake.custom_fake = headers_init_custom_fake;
    headers_set_fake.return_val = 0;
    h2_send_request_fake.return_val = 0;
    h2_receive_frame_fake.return_val = 0;
    headers_get_fake.return_val = "200";

    uint8_t *res = (uint8_t *)malloc(5);
    size_t size_init = sizeof(res);
    size_t size_end = sizeof(res);

    int scr = send_client_request(&hs, "GET", "/index", res, &size_end);

    TEST_ASSERT_EQUAL(2, headers_init_fake.call_count);
    TEST_ASSERT_EQUAL(4, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_send_request_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_receive_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, headers_get_fake.call_count);

    TEST_ASSERT_EQUAL(200, scr);

    TEST_ASSERT_EQUAL((int)size_init, (int)size_end);

    free(res);
}


void test_send_client_request_fail_receive_server_response(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.connection_state = 0;
    hs.hd_lists.data_in_size = 0;
    hs.keep_receiving = 0;

    headers_init_fake.custom_fake = headers_init_custom_fake;
    headers_set_fake.return_val = 0;
    h2_send_request_fake.return_val = 0;

    uint8_t *res = (uint8_t *)malloc(5);
    size_t size = sizeof(res);

    int scr = send_client_request(&hs, "GET", "/index", res, &size);

    TEST_ASSERT_EQUAL(2, headers_init_fake.call_count);
    TEST_ASSERT_EQUAL(4, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_send_request_fake.call_count);

    TEST_ASSERT_EQUAL(-1, scr);

    free(res);
}


void test_send_client_request_fail_h2_send_request(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.connection_state = 0;
    hs.hd_lists.data_in_size = 0;
    hs.keep_receiving = 0;

    headers_set_fake.return_val = 0;
    h2_send_request_fake.return_val = -1;

    uint8_t *res = (uint8_t *)malloc(5);
    size_t size = sizeof(res);

    int scr = send_client_request(&hs, "GET", "/index", res, &size);

    TEST_ASSERT_EQUAL(4, headers_set_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_send_request_fake.call_count);

    TEST_ASSERT_EQUAL(-1, scr);

    free(res);
}


void test_send_client_request_fail_headers_set(void)
{
    hstates_t hs;

    reset_http_states(&hs);
    hs.connection_state = 0;
    hs.hd_lists.data_in_size = 0;
    hs.keep_receiving = 0;

    headers_set_fake.return_val = -1;

    uint8_t *res = (uint8_t *)malloc(5);
    size_t size = sizeof(res);

    int scr = send_client_request(&hs, "GET", "/index", res, &size);

    TEST_ASSERT_EQUAL(1, headers_set_fake.call_count);

    TEST_ASSERT_EQUAL(-1, scr);

    free(res);
}


void test_http_client_connect_success(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = 0;
    h2_client_init_connection_fake.return_val = 0;

    int cc = http_client_connect(&hs, "::1", 8888);

    TEST_ASSERT_EQUAL(1, sock_create_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_connect_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_client_init_connection_fake.call_count);

    TEST_ASSERT_EQUAL(0, cc);

    TEST_ASSERT_EQUAL(0, hs.is_server);
    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
    TEST_ASSERT_EQUAL(0, strncmp((char *)hs.host, "::1", HTTP_MAX_HOST_SIZE));
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
}


void test_http_client_connect_fail_h2_client_init_connection(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = 0;
    h2_client_init_connection_fake.return_val = -1;

    int cc = http_client_connect(&hs, "::1", 8888);

    TEST_ASSERT_EQUAL(1, sock_create_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_connect_fake.call_count);
    TEST_ASSERT_EQUAL(1, h2_client_init_connection_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Problems sending client data");

    TEST_ASSERT_EQUAL(1, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.headers_in.count);
    TEST_ASSERT_EQUAL(1, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
}


void test_http_client_connect_fail_sock_connect(void)
{
    hstates_t hs;

    sock_create_fake.return_val = 0;
    sock_connect_fake.return_val = -1;
    sock_destroy_fake.return_val = 0;

    int cc = http_client_connect(&hs, "::1", 8888);

    TEST_ASSERT_EQUAL(1, sock_create_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_connect_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_destroy_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client connection");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.headers_in.count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
}


void test_http_client_connect_fail_sock_create(void)
{
    hstates_t hs;

    sock_create_fake.return_val = -1;

    int cc = http_client_connect(&hs, "::1", 8888);

    TEST_ASSERT_EQUAL(1, sock_create_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client creation");

    TEST_ASSERT_EQUAL(0, hs.socket_state);
    TEST_ASSERT_EQUAL(0, hs.server_socket_state);
    TEST_ASSERT_EQUAL(0, hs.headers_in.count);
    TEST_ASSERT_EQUAL(0, hs.connection_state);
    TEST_ASSERT_EQUAL(0, hs.is_server);
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


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_get_data_success);
    UNIT_TEST(test_get_data_fail_no_data);

    UNIT_TEST(test_set_data_success);
    UNIT_TEST(test_set_data_fail_big_data);
    UNIT_TEST(test_set_data_fail_data_full);

    UNIT_TEST(test_reset_http_states_success);

    UNIT_TEST(test_do_request_success);
    UNIT_TEST(test_do_request_fail_h2_send_response);
    UNIT_TEST(test_do_request_path_not_found);
    UNIT_TEST(test_do_request_no_resources);

    UNIT_TEST(test_http_server_create_success);
    UNIT_TEST(test_http_server_create_fail_sock_listen);
    UNIT_TEST(test_http_server_create_fail_sock_create);

    //UNIT_TEST(test_http_server_start_success);
    UNIT_TEST(test_http_server_start_fail_h2_server_init_connection);
    UNIT_TEST(test_http_server_start_fail_sock_accept);

    UNIT_TEST(test_http_server_destroy_success);
    UNIT_TEST(test_http_server_destroy_success_without_client);
    UNIT_TEST(test_http_server_destroy_fail_not_server);
    UNIT_TEST(test_http_server_destroy_fail_sock_destroy);

    UNIT_TEST(test_http_register_resource_success_v1);
    UNIT_TEST(test_http_register_resource_success_v2);
    UNIT_TEST(test_http_register_resource_fail_invalid_input);
    UNIT_TEST(test_http_register_resource_fail_not_supported_method);
    UNIT_TEST(test_http_register_resource_fail_invalid_path);
    UNIT_TEST(test_http_register_resource_fail_list_full);

    UNIT_TEST(test_receive_server_response_success);
    UNIT_TEST(test_receive_server_response_fail_h2_receive_frame);
    UNIT_TEST(test_receive_server_response_fail_connection_state);

    UNIT_TEST(test_send_client_request_success);
    UNIT_TEST(test_send_client_request_fail_receive_server_response);
    UNIT_TEST(test_send_client_request_fail_h2_send_request);
    UNIT_TEST(test_send_client_request_fail_headers_set);

    UNIT_TEST(test_http_client_connect_success);
    UNIT_TEST(test_http_client_connect_fail_h2_client_init_connection);
    UNIT_TEST(test_http_client_connect_fail_sock_connect);
    UNIT_TEST(test_http_client_connect_fail_sock_create);

    UNIT_TEST(test_http_client_disconnect_success_v1);
    UNIT_TEST(test_http_client_disconnect_success_v2);
    UNIT_TEST(test_http_client_disconnect_fail);

    return UNITY_END();
}
