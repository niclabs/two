#include <arpa/inet.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>

#include "cbuf.h"
#include "event.h"
//#define LOG_LEVEL (LOG_LEVEL_DEBUG)
#include "fff.h"
#include "unit.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, bind, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, close, int);
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, setsockopt, int, int, int, const void *, socklen_t);
FAKE_VALUE_FUNC(int,
                select,
                int,
                fd_set *,
                fd_set *,
                fd_set *,
                struct timeval *);
FAKE_VALUE_FUNC(ssize_t, recv, int, void *, size_t, int);
FAKE_VALUE_FUNC(ssize_t, send, int, const void *, size_t, int);
FAKE_VOID_FUNC(cbuf_init, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_peek, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_pop, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int, cbuf_len, cbuf_t *);
FAKE_VALUE_FUNC(int, cbuf_maxlen, cbuf_t *);
FAKE_VOID_FUNC(cbuf_end, cbuf_t *);
FAKE_VALUE_FUNC(int, cbuf_has_ended, cbuf_t *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                                                   \
    FAKE(socket)                                                               \
    FAKE(bind)                                                                 \
    FAKE(listen)                                                               \
    FAKE(close)                                                                \
    FAKE(accept)                                                               \
    FAKE(setsockopt)                                                           \
    FAKE(recv)                                                                 \
    FAKE(send)                                                                 \
    FAKE(cbuf_init)                                                            \
    FAKE(cbuf_push)                                                            \
    FAKE(cbuf_peek)                                                            \
    FAKE(cbuf_pop)                                                             \
    FAKE(cbuf_len)                                                             \
    FAKE(cbuf_end)                                                             \
    FAKE(cbuf_has_ended)                                                       \
    FAKE(cbuf_maxlen)                                                          \
    FAKE(select)

int fake_cbuf_len;
int fake_cbuf_ended;
uint8_t buf[32];

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();

    // reset cbuf len
    fake_cbuf_len   = 0;
    fake_cbuf_ended = 0;
    memset(buf, 0, 32);
}

//////////////////////////////////////////////////////////////////////////
// fake functions and generic callbacks
//////////////////////////////////////////////////////////////////////////

int select_with_no_activity(int nfds,
                            fd_set *read_set,
                            fd_set *write_set,
                            fd_set *except_set,
                            struct timeval *tv)
{
    // Reset read activity
    FD_CLR(0, read_set);
    FD_CLR(1, read_set);
    FD_CLR(2, read_set);

    // Reset write activity
    FD_CLR(0, write_set);
    FD_CLR(1, write_set);
    FD_CLR(2, write_set);
    return 0;
}

// Simulate a read event on socket 1
int select_with_read_on_s1_fake(int nfds,
                                fd_set *read_set,
                                fd_set *write_set,
                                fd_set *except_set,
                                struct timeval *tv)
{
    TEST_ASSERT_GREATER_THAN(1, nfds);

    // Reset read activity
    FD_CLR(0, read_set);
    FD_CLR(1, read_set);
    FD_CLR(2, read_set);

    // Reset write activity
    FD_CLR(0, write_set);
    FD_CLR(1, write_set);
    FD_CLR(2, write_set);

    // set activity on the socket 1
    FD_SET(1, read_set);
    return 0;
}

// Simulate a read event on socket 2
int select_with_read_on_s2_fake(int nfds,
                                fd_set *read_set,
                                fd_set *write_set,
                                fd_set *except_set,
                                struct timeval *tv)
{
    TEST_ASSERT_GREATER_THAN(2, nfds);

    // Reset read activity
    FD_CLR(0, read_set);
    FD_CLR(1, read_set);
    FD_CLR(2, read_set);

    // Reset write activity
    FD_CLR(0, write_set);
    FD_CLR(1, write_set);
    FD_CLR(2, write_set);

    // set activity on the socket 1
    FD_SET(2, read_set);
    return 0;
}

// Simulate a read event on socket 2
int select_with_write_on_s2_fake(int nfds,
                                 fd_set *read_set,
                                 fd_set *write_set,
                                 fd_set *except_set,
                                 struct timeval *tv)
{
    TEST_ASSERT_GREATER_THAN(2, nfds);

    // Reset read activity
    FD_CLR(0, read_set);
    FD_CLR(1, read_set);
    FD_CLR(2, read_set);

    // Reset write activity
    FD_CLR(0, write_set);
    FD_CLR(1, write_set);
    FD_CLR(2, write_set);

    // set activity on the socket 1
    FD_SET(2, write_set);
    return 0;
}

// Generic close calback check
void close_s1_cb(event_sock_t *sock)
{
    TEST_ASSERT_EQUAL_MESSAGE(
      1, sock->descriptor, "closed socket must be equal to 1");
}

// Generic close calback check
void close_s2_cb(event_sock_t *sock)
{
    TEST_ASSERT_EQUAL_MESSAGE(
      2, sock->descriptor, "closed socket must be equal to 2");
}

ssize_t recv_nothing(int s, void *dst, size_t len, int flags)
{
    errno = EWOULDBLOCK;
    return -1;
}

ssize_t recv_hello_world(int s, void *dst, size_t len, int flags)
{
    TEST_ASSERT_EQUAL(2, s);
    TEST_ASSERT_EQUAL(MSG_DONTWAIT, flags);
    TEST_ASSERT_GREATER_OR_EQUAL(13, len);

    memcpy(dst, "Hello, World!", 13);

    return 13;
}

ssize_t send_world(int s, const void *src, size_t len, int flags)
{
    TEST_ASSERT_EQUAL(2, s);
    TEST_ASSERT_EQUAL(MSG_DONTWAIT, flags);
    TEST_ASSERT_GREATER_OR_EQUAL(8, len);
    TEST_ASSERT_EQUAL_STRING_LEN(", World!", src, 8);

    return 8;
}

ssize_t send_hello(int s, const void *src, size_t len, int flags)
{
    TEST_ASSERT_EQUAL(2, s);
    TEST_ASSERT_EQUAL(MSG_DONTWAIT, flags);
    TEST_ASSERT_GREATER_OR_EQUAL(13, len);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, World!", src, 13);

    return 5;
}

int test_cbuf_len(cbuf_t *cbuf)
{
    return fake_cbuf_len;
}

int test_cbuf_maxlen(cbuf_t *cbuf)
{
    return 32;
}

int test_cbuf_pop(cbuf_t *cb, void *dst, int size)
{
    TEST_ASSERT_GREATER_OR_EQUAL(size, fake_cbuf_len);

    char *str = "Hello, World!";
    if (dst != NULL) {
        memcpy(dst, str + 13 - size, size);
    }

    fake_cbuf_len -= size;
    return size;
}

int test_cbuf_push(cbuf_t *cb, void *dst, int size)
{
    if (size > 0 && !fake_cbuf_ended) {
        fake_cbuf_len += size;
        return size;
    }
    return 0;
}

int test_cbuf_peek(cbuf_t *cb, void *dst, int size)
{
    TEST_ASSERT_GREATER_OR_EQUAL(size, fake_cbuf_len);

    char *str = "Hello, World!";
    memcpy(dst, str + 13 - size, size);

    return size;
}

void test_cbuf_end(cbuf_t *cb)
{
    fake_cbuf_ended = 1;
}

int test_cbuf_has_ended(cbuf_t *cb)
{
    return fake_cbuf_ended == 1;
}

//////////////////////////////////////////////////////////////////////////
// test_event_listen
//////////////////////////////////////////////////////////////////////////

event_sock_t *test_event_listen_ok_cb(event_sock_t *server)
{
    TEST_ASSERT_EQUAL_MESSAGE(
      1, server->descriptor, "listen socket must be equal to 1");

    // close socket
    event_close(server, close_s1_cb);

    return NULL;
}

void test_event_listen(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
      NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    select_fake.custom_fake = select_with_read_on_s1_fake;
    socket_fake.return_val  = 1;

    // configure sock as server socket
    event_listen(sock, 8888, test_event_listen_ok_cb);

    TEST_ASSERT_EQUAL_MESSAGE(
      1, socket_fake.call_count, "socket should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(
      1, bind_fake.call_count, "bind should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(
      htons(8888),
      ((struct sockaddr_in6 *)bind_fake.arg1_val)->sin6_port,
      "bind should be called with port 8888");
    TEST_ASSERT_EQUAL_MESSAGE(
      1, listen_fake.call_count, "listen should be called once");

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(
      1, select_fake.call_count, "select should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS,
                              event_sock_unused(&loop),
                              "all sockets should be unused after loop finish");
}

//////////////////////////////////////////////////////////////////////////
// test_event_listen_no_more_sockets
//////////////////////////////////////////////////////////////////////////

event_sock_t *test_event_listen_no_more_sockets_cb(event_sock_t *server)
{
    TEST_ASSERT_EQUAL(1, server->descriptor);

    // close socket
    event_close(server, close_s1_cb);

    return NULL;
}

void test_event_listen_on_close(event_sock_t *sock)
{
    (void)sock;
}

int test_event_listen_timeout(event_sock_t *sock)
{
    event_close(sock, test_event_listen_on_close);
    return 1;
}

void test_event_listen_no_sockets_available(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
      NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    int (*select_fakes[])(
      int, fd_set *, fd_set *, fd_set *, struct timeval *) = {
        select_with_read_on_s1_fake,
        select_with_read_on_s1_fake,
    };
    SET_CUSTOM_FAKE_SEQ(select, select_fakes, 2);
    socket_fake.return_val = 1;

    // configure sock as server socket
    event_listen(sock, 8888, test_event_listen_no_more_sockets_cb);

    TEST_ASSERT_EQUAL_MESSAGE(
      1, socket_fake.call_count, "socket should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(
      1, bind_fake.call_count, "bind should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(
      htons(8888),
      ((struct sockaddr_in6 *)bind_fake.arg1_val)->sin6_port,
      "bind should be called with port 8888");
    TEST_ASSERT_EQUAL_MESSAGE(
      1, listen_fake.call_count, "listen should be called once");

    // remove other sockets
    for (int i = 0; i < EVENT_MAX_SOCKETS - 1; i++) {
        event_sock_t *client = event_sock_create(&loop);
        // Set a timeout to close the socket immediately
        TEST_ASSERT_NOT_EQUAL(
          NULL, event_timer_set(client, 0, test_event_listen_timeout));
    }

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(
      2, select_fake.call_count, "select should be called twice");
    TEST_ASSERT_EQUAL_MESSAGE(
      EVENT_MAX_SOCKETS,
      event_sock_unused(&loop),
      "server socket should be unused after loop finish");
}

//////////////////////////////////////////////////////////////////////////
// test_event_accept
//////////////////////////////////////////////////////////////////////////
event_sock_t *test_event_accept_listen_cb(event_sock_t *server)
{
    TEST_ASSERT_EQUAL(1, server->descriptor);
    TEST_ASSERT_NOT_EQUAL(NULL, server->loop);

    event_sock_t *client = event_sock_create(server->loop);

    // set accept return value
    accept_fake.return_val = 2;
    TEST_ASSERT_EQUAL(0, event_accept(server, client));
    TEST_ASSERT_EQUAL(1, accept_fake.call_count);
    TEST_ASSERT_EQUAL(2, client->descriptor);
    TEST_ASSERT_EQUAL(EVENT_SOCK_CONNECTED, client->state);

    // close sockets
    event_close(client, close_s2_cb);
    event_close(server, close_s1_cb);

    return client;
}

void test_event_accept(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
      NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    int (*select_fakes[])(
      int, fd_set *, fd_set *, fd_set *, struct timeval *) = {
        select_with_read_on_s1_fake, select_with_no_activity
    };
    SET_CUSTOM_FAKE_SEQ(select, select_fakes, 2);
    socket_fake.return_val = 1;

    // configure sock as server socket
    event_listen(sock, 8888, test_event_accept_listen_cb);

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(
      1, select_fake.call_count, "select should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS,
                              event_sock_unused(&loop),
                              "all sockets should be unused after loop finish");
}

//////////////////////////////////////////////////////////////////////////
// test_event_read
//////////////////////////////////////////////////////////////////////////
int test_event_read_world_cb(struct event_sock *sock, int size, uint8_t *bytes)
{
    TEST_ASSERT_EQUAL(8, size);
    TEST_ASSERT_EQUAL_STRING_LEN(", World!", bytes, 8);

    event_close(sock, close_s2_cb);

    return 8;
}

int test_event_read_hello_cb(struct event_sock *sock, int size, uint8_t *bytes)
{
    TEST_ASSERT_EQUAL(13, size);
    TEST_ASSERT_EQUAL_STRING_LEN("Hello, World!", bytes, 13);

    event_read(sock, test_event_read_world_cb);

    return 5;
}

event_sock_t *test_event_read_listen_cb(event_sock_t *server)
{
    TEST_ASSERT_EQUAL(1, server->descriptor);
    TEST_ASSERT_NOT_EQUAL(NULL, server->loop);

    event_sock_t *client = event_sock_create(server->loop);

    // set accept return value
    accept_fake.return_val = 2;
    TEST_ASSERT_EQUAL(0, event_accept(server, client));
    TEST_ASSERT_EQUAL(2, client->descriptor);

    // call read start
    event_read_start(client, buf, 32, test_event_read_hello_cb);

    // close sockets
    event_close(server, close_s1_cb);

    return client;
}

void test_event_read(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
      NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    int (*select_fakes[])(int,
                          fd_set *,
                          fd_set *,
                          fd_set *,
                          struct timeval *) = { select_with_read_on_s1_fake,
                                                select_with_read_on_s2_fake,
                                                select_with_no_activity };
    SET_CUSTOM_FAKE_SEQ(select, select_fakes, 3);

    // The call to socket should return the server socket value
    socket_fake.return_val = 1;

    // buffer responses
    cbuf_peek_fake.custom_fake      = test_cbuf_peek;
    cbuf_pop_fake.custom_fake       = test_cbuf_pop;
    cbuf_len_fake.custom_fake       = test_cbuf_len;
    cbuf_maxlen_fake.custom_fake    = test_cbuf_maxlen;
    cbuf_push_fake.custom_fake      = test_cbuf_push;
    cbuf_end_fake.custom_fake       = test_cbuf_end;
    cbuf_has_ended_fake.custom_fake = test_cbuf_has_ended;

    // read "Hello, World!" on first read and nothing after
    ssize_t (*recv_fakes[])(int, void *, size_t, int) = { recv_hello_world,
                                                          recv_nothing };
    SET_CUSTOM_FAKE_SEQ(recv, recv_fakes, 2);

    // configure sock as server socket
    event_listen(sock, 8888, test_event_read_listen_cb);

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL(3, select_fake.call_count);
    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS,
                              event_sock_unused(&loop),
                              "all sockets should be unused after loop finish");
}

//////////////////////////////////////////////////////////////////////////
// test_event_write
//////////////////////////////////////////////////////////////////////////
void test_event_write_hello_world_cb(struct event_sock *client, int status)
{
    TEST_ASSERT_EQUAL(2, client->descriptor);
    TEST_ASSERT_EQUAL(0, status);

    // close sockets
    event_close(client, close_s2_cb);
}

event_sock_t *test_event_write_listen_cb(event_sock_t *server)
{
    TEST_ASSERT_EQUAL(1, server->descriptor);
    TEST_ASSERT_NOT_EQUAL(NULL, server->loop);

    event_sock_t *client = event_sock_create(server->loop);

    TEST_ASSERT_EQUAL(0, event_accept(server, client));
    TEST_ASSERT_EQUAL(2, client->descriptor);

    // enable event write
    event_write_enable(client, buf, 32);

    // call write
    event_write(client,
                13,
                (unsigned char *)"Hello, World!",
                test_event_write_hello_world_cb);

    // close sockets
    event_close(server, close_s1_cb);

    return client;
}

void test_event_write(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    event_sock_t *sock = event_sock_create(&loop);
    TEST_ASSERT_NOT_EQUAL_MESSAGE(
      NULL, sock, "result of sock_create cannot return null");

    // set fake functions
    int (*select_fakes[])(
      int, fd_set *, fd_set *, fd_set *, struct timeval *) = {
        select_with_read_on_s1_fake, select_with_write_on_s2_fake
    };
    SET_CUSTOM_FAKE_SEQ(select, select_fakes, 2);

    // The call to socket should return the server socket value
    socket_fake.return_val = 1;

    // set accept return value
    accept_fake.return_val = 2;

    // buffer responses
    cbuf_peek_fake.custom_fake      = test_cbuf_peek;
    cbuf_pop_fake.custom_fake       = test_cbuf_pop;
    cbuf_len_fake.custom_fake       = test_cbuf_len;
    cbuf_maxlen_fake.custom_fake    = test_cbuf_maxlen;
    cbuf_push_fake.custom_fake      = test_cbuf_push;
    cbuf_end_fake.custom_fake       = test_cbuf_end;
    cbuf_has_ended_fake.custom_fake = test_cbuf_has_ended;

    // read "Hello, World!" on first read and nothing after
    ssize_t (*send_fakes[])(int, const void *, size_t, int) = { send_hello,
                                                                send_world };
    SET_CUSTOM_FAKE_SEQ(send, send_fakes, 2);

    // configure sock as server socket
    event_listen(sock, 8888, test_event_write_listen_cb);

    // start loop
    event_loop(&loop);

    TEST_ASSERT_EQUAL(3, select_fake.call_count);
    TEST_ASSERT_EQUAL_MESSAGE(EVENT_MAX_SOCKETS,
                              event_sock_unused(&loop),
                              "all sockets should be unused after loop finish");
}
//////////////////////////////////////////////////////////////////////////
// test_event_sock_create
//////////////////////////////////////////////////////////////////////////

void test_event_sock_create(void)
{
    event_loop_t loop;

    event_loop_init(&loop);

    TEST_ASSERT_EQUAL_MESSAGE(
      EVENT_MAX_SOCKETS,
      event_sock_unused(&loop),
      "unused sockets should equal EVENT_MAX_SOCKETS after loop creation");
    for (int i = 0; i < EVENT_MAX_SOCKETS; i++) {
        event_sock_create(&loop);
    }

    TEST_ASSERT_EQUAL_MESSAGE(
      0,
      event_sock_unused(&loop),
      "only EVENT_MAX_SOCKETS can be created with event_sock_create");
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_event_sock_create);
    UNIT_TEST(test_event_listen);
    UNIT_TEST(test_event_listen_no_sockets_available);
    UNIT_TEST(test_event_accept);
    UNIT_TEST(test_event_read);
    UNIT_TEST(test_event_write);
    UNIT_TESTS_END();
}
