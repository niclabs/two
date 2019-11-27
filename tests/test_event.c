#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include "unit.h"
#include "event.h"
#include "cbuf.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, bind, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, close, int);
FAKE_VALUE_FUNC(int, setsockopt, int, int, int, const void*, socklen_t);
FAKE_VALUE_FUNC(int, select, int, fd_set *, fd_set *, fd_set *, struct timeval *);
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_peek, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_pop, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_len, cbuf_t *);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)    \
    FAKE(socket)                \
    FAKE(bind)                  \
    FAKE(listen)                \
    FAKE(close)                 \
    FAKE(setsockopt)            \
    FAKE(cbuf_push)            \
    FAKE(cbuf_peek)            \
    FAKE(cbuf_pop)            \
    FAKE(cbuf_len)            \
    FAKE(select)


void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

int test_event_listen_socket_fake(int domain, int type, int protocol)
{
    return 1;
}

int test_event_listen_select_fake(int nfds, fd_set *read_set, fd_set *write_set, fd_set *except_set, struct timeval *tv)
{
    TEST_ASSERT_GREATER_THAN_MESSAGE(1, nfds, "nfds given to select must be greater than 1");

    // Set read activity on socket
    FD_SET(1, read_set);
    return 0;
}

void test_event_listen_close_cb(event_sock_t *sock)
{
    TEST_ASSERT_EQUAL_MESSAGE(1, sock->descriptor, "closed socket must be equal to 1");
}

void test_event_listen_accept_cb(struct event_sock *server, int status)
{
    TEST_ASSERT_EQUAL_MESSAGE(1, server->descriptor, "listen socket must be equal to 1");
    TEST_ASSERT_EQUAL(0, status);

    // close socket
    event_close(server, test_event_listen_close_cb);
}


void test_event_listen_accept_no_more_sockets_cb(struct event_sock *server, int status)
{
    TEST_ASSERT_EQUAL_MESSAGE(1, server->descriptor, "listen socket must be equal to 1");
    TEST_ASSERT_EQUAL_MESSAGE(-1, status, "accept status should be error if no more clients are available");

    // close socket
    event_close(server, test_event_listen_close_cb);
}

void test_event_listen(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    select_fake.custom_fake = test_event_listen_select_fake;
    socket_fake.custom_fake = test_event_listen_socket_fake;

    // configure sock as server socket
    event_listen(sock, 8888, test_event_listen_accept_cb);

    TEST_ASSERT_EQUAL_MESSAGE(1, socket_fake.call_count, "socket should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, bind_fake.call_count, "bind should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(htons(8888), ((struct sockaddr_in6 *)bind_fake.arg1_val)->sin6_port, "bind should be called with port 8888");
    TEST_ASSERT_EQUAL_MESSAGE(1, listen_fake.call_count, "listen should be called once");

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(1, select_fake.call_count, "select should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS, event_sock_unused(&loop), "all sockets should be unused after loop finish");
}

void test_event_listen_no_sockets_available(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    select_fake.custom_fake = test_event_listen_select_fake;
    socket_fake.custom_fake = test_event_listen_socket_fake;

    // configure sock as server socket
    event_listen(sock, 8888, test_event_listen_accept_no_more_sockets_cb);

    TEST_ASSERT_EQUAL_MESSAGE(1, socket_fake.call_count, "socket should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, bind_fake.call_count, "bind should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(htons(8888), ((struct sockaddr_in6 *)bind_fake.arg1_val)->sin6_port, "bind should be called with port 8888");
    TEST_ASSERT_EQUAL_MESSAGE(1, listen_fake.call_count, "listen should be called once");

    // remove other sockets
    for (int i = 0; i < EVENT_MAX_SOCKETS - 1; i++) {
        event_sock_create(&loop);
    }

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(1, select_fake.call_count, "select should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, event_sock_unused(&loop), "server socket should be unused after loop finish");
}

void test_event_sock_create(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS, event_sock_unused(&loop), "unused sockets should equal EVENT_MAX_SOCKETS after loop creation");
    for (int i = 0; i < EVENT_MAX_SOCKETS; i++) {
        event_sock_create(&loop);
    }

    TEST_ASSERT_EQUAL_MESSAGE(0, event_sock_unused(&loop), "only EVENT_MAX_SOCKETS can be created with event_sock_create");
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_event_sock_create);
    UNIT_TEST(test_event_listen);
    UNIT_TEST(test_event_listen_no_sockets_available);
    UNIT_TESTS_END();
}
