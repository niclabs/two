#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http_methods.h"
#include "http_methods_bridge.h"


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
FAKE_VALUE_FUNC(int, client_init_connection, hstates_t *);
FAKE_VALUE_FUNC(int, server_init_connection, hstates_t *);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)            \
    FAKE(sock_create)                     \
    FAKE(sock_listen)                     \
    FAKE(sock_accept)                     \
    FAKE(sock_connect)                    \
    FAKE(sock_read)                       \
    FAKE(sock_write)                      \
    FAKE(sock_destroy)                    \
    FAKE(client_init_connection)          \
    FAKE(server_init_connection)          \


void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


int sock_create_custom_fake(sock_t *s)
{
    s->fd = 2;
    s->state = SOCK_OPENED;
    return 0;
}
int sock_listen_custom_fake(sock_t *s, uint16_t n)
{
    s->state = SOCK_LISTENING;
    return 0;
}
int sock_accept_custom_fake(sock_t *ss, sock_t *sc)
{
    sc->fd = 2;
    sc->state = SOCK_CONNECTED;
    return 0;
}
int sock_connect_custom_fake(sock_t *s, char *ip, uint16_t p)
{
    s->state = SOCK_CONNECTED;
    return 0;
}
int sock_read_custom_fake(sock_t *s, char *buf, int l, int u)
{
    strcpy(buf, "hola");
    (void)s;
    (void)u;
    return 4;
}
int sock_destroy_custom_fake(sock_t *s)
{
    s->state = SOCK_CLOSED;
    return 0;
}


void test_http_init_server_success(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.custom_fake = sock_accept_custom_fake;
    server_init_connection_fake.return_val = 0;

    int is = http_init_server(12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

    TEST_ASSERT_EQUAL(0, is);
}


void test_http_init_server_fail_server_init_connection(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.custom_fake = sock_accept_custom_fake;
    server_init_connection_fake.return_val = -1;

    int is = http_init_server(12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");
}


void test_http_init_server_fail_sock_accept(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.return_val = -1;

    int is = http_init_server(12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");
}


void test_http_init_server_fail_sock_listen(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.return_val = -1;

    int is = http_init_server(12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Partial error in server creation");
}


void test_http_init_server_fail_sock_create(void)
{
    sock_create_fake.return_val = -1;

    int is = http_init_server(12);

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Server could not be created");
}


void test_http_server_destroy_success(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.custom_fake = sock_accept_custom_fake;
    server_init_connection_fake.return_val = 0;

    http_init_server(12);


    sock_destroy_fake.custom_fake = sock_destroy_custom_fake;

    int d = http_server_destroy();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[4]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[5]);

    TEST_ASSERT_EQUAL(0, d);
}


void test_http_server_destroy_success_without_client(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.return_val = -1;

    http_init_server(12);


    sock_destroy_fake.custom_fake = sock_destroy_custom_fake;

    int d = http_server_destroy();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);

    TEST_ASSERT_EQUAL(0, d);
}


void test_http_server_destroy_fail_not_server(void)
{
    sock_create_fake.return_val = -1;

    http_client_connect(12, "::");

    int d = http_server_destroy();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Server not found");
}


void test_http_server_destroy_fail_sock_destroy(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_listen_fake.custom_fake = sock_listen_custom_fake;
    sock_accept_fake.custom_fake = sock_accept_custom_fake;
    server_init_connection_fake.return_val = 0;

    http_init_server(12);


    int returnVals[2] = { 0, -1 };
    SET_RETURN_SEQ(sock_destroy, returnVals, 2);

    int d = http_server_destroy();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
    TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[4]);
    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[5]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Error in server disconnection");
}



void test_http_client_connect_success(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.custom_fake = sock_connect_custom_fake;
    client_init_connection_fake.return_val = 0;

    int cc = http_client_connect(12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL(0, cc);
}


void test_http_client_connect_fail_client_init_connection(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.custom_fake = sock_connect_custom_fake;
    client_init_connection_fake.return_val = -1;

    int cc = http_client_connect(12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Problems sending client data");
}


void test_http_client_connect_fail_sock_connect(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.return_val = -1;

    int cc = http_client_connect(12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client connection");
}


void test_http_client_connect_fail_sock_create(void)
{
    sock_create_fake.return_val = -1;

    int cc = http_client_connect(12, "::");

    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client creation");
}


void test_http_client_disconnect_success_v1(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.return_val = -1;

    http_client_connect(12, "::");

    int d = http_client_disconnect();

    TEST_ASSERT_EQUAL(0, d);
}


void test_http_client_disconnect_success_v2(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.custom_fake = sock_connect_custom_fake;
    client_init_connection_fake.return_val = 0;

    http_client_connect(12, "::");


    sock_destroy_fake.custom_fake = sock_destroy_custom_fake;

    int d = http_client_disconnect();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);

    TEST_ASSERT_EQUAL(0, d);
}


void test_http_client_disconnect_fail(void)
{
    sock_create_fake.custom_fake = sock_create_custom_fake;
    sock_connect_fake.custom_fake = sock_connect_custom_fake;
    client_init_connection_fake.return_val = 0;

    http_client_connect(12, "::");


    sock_destroy_fake.return_val = -1;

    int d = http_client_disconnect();


    TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
    TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
    TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

    TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);

    TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Client could not be disconnected");
}


int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_http_init_server_success);
    UNIT_TEST(test_http_init_server_fail_server_init_connection);
    UNIT_TEST(test_http_init_server_fail_sock_accept);
    UNIT_TEST(test_http_init_server_fail_sock_listen);
    UNIT_TEST(test_http_init_server_fail_sock_create);

    UNIT_TEST(test_http_server_destroy_success);
    UNIT_TEST(test_http_server_destroy_success_without_client);
    UNIT_TEST(test_http_server_destroy_fail_not_server);
    UNIT_TEST(test_http_server_destroy_fail_sock_destroy);

    UNIT_TEST(test_http_client_connect_success);
    UNIT_TEST(test_http_client_connect_fail_client_init_connection);
    UNIT_TEST(test_http_client_connect_fail_sock_connect);
    UNIT_TEST(test_http_client_connect_fail_sock_create);

    UNIT_TEST(test_http_client_disconnect_success_v1);
    UNIT_TEST(test_http_client_disconnect_success_v2);
    UNIT_TEST(test_http_client_disconnect_fail);

    return UNITY_END();
}
