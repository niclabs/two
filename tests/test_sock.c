#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>

#include "unit.h"
#include "sock.h"
#include "fff.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, bind, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, close, int);
FAKE_VALUE_FUNC(int, connect, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, select, int, fd_set *, fd_set *, fd_set *, struct timeval *);
FAKE_VALUE_FUNC(ssize_t, recv, int, void *, size_t, int);
FAKE_VALUE_FUNC(ssize_t, send, int, const void *, size_t, int);
FAKE_VALUE_FUNC(int, setsockopt, int, int, int, const void*, socklen_t);
FAKE_VALUE_FUNC(int, ioctl, int, unsigned long, ssize_t*);
FAKE_VALUE_FUNC(int, fcntl, int, int, int);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) \
    FAKE(socket)             \
    FAKE(bind)               \
    FAKE(listen)             \
    FAKE(accept)             \
    FAKE(close)              \
    FAKE(select)             \
    FAKE(recv)               \
    FAKE(send)               \
    FAKE(connect)            \
    FAKE(setsockopt)         \
    FAKE(ioctl)              \
    FAKE(fcntl)


char global_write_buffer[64];
void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();

    // Reset global write buffer
    memset(global_write_buffer, 0, 64);
}

/**************************************************************************
 * posix socket mocks
 *************************************************************************/
int socket_with_error_fake(int domain, int type, int protocol)
{
    errno = EAFNOSUPPORT;
    return -1;
}

int bind_with_error_fake(int sockfd, const struct sockaddr *addr,
                         socklen_t addrlen)
{
    errno = ENOTSOCK;
    return -1;
}

int listen_with_error_fake(int sockfd, int backlog)
{
    errno = EOPNOTSUPP;
    return -1;
}

int accept_with_error_fake(int sockfd, struct sockaddr *addr, socklen_t *addrlen)
{
    errno = EBADF;
    return -1;
}


int connect_with_error_fake(int sockfd, const struct sockaddr *addr, socklen_t addrlen)
{
    errno = ECONNREFUSED;
    return -1;
}

ssize_t recv_ok_fake(int fd, void *buf, size_t count, int flags)
{
    (void)flags;
    strncpy(buf, "HELLO WORLD", 12);
    return 12;
}

ssize_t recv_zero_bytes_fake(int fd, void *buf, size_t count, int flags)
{
    (void)flags;
    return 0;
}

ssize_t send_ok_fake(int fd, const void *buf, size_t count, int flags)
{
    (void)flags;
    // write to global buffer
    memcpy(global_write_buffer, buf, count);
    return count;
}

ssize_t send_with_error_fake(int fd, const void *buf, size_t count, int flags)
{
    (void)flags;
    errno = EBADF;
    return -1;
}

int close_with_error_fake(int fd)
{
    errno = EBADF;
    return -1;
}


/**************************************************************************
 * sock_create tests
 *************************************************************************/

void test_sock_create_ok(void)
{
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_OPENED, sock.state, "sock_create should leave sock in 'OPENED' state");
    TEST_ASSERT_EQUAL_MESSAGE(123, sock.socket, "sock_create should set file descriptor in sock structure");
    sock_destroy(&sock);
}

void test_sock_create_fail_to_create_socket(void)
{
    sock_t sock;

    socket_fake.custom_fake = socket_with_error_fake;
    int res = sock_create(&sock);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_create should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_create should set errno on error");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CLOSED, sock.state, "sock_create should leave sock in 'CLOSED' state on error");
}

/**************************************************************************
 * sock_listen tests
 *************************************************************************/

void test_sock_listen_unitialized_socket(void)
{
    sock_t sock;

    listen_fake.return_val = -1;
    int res = sock_listen(&sock, 8888);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_listen on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_listen should set errno on error");
}

void test_sock_listen_null_socket(void)
{
    listen_fake.return_val = -1;
    int res = sock_listen(NULL, 8888);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_listen on NULL socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_listen should set errno on error");
}

void test_sock_listen_ok(void)
{
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    listen_fake.return_val = 0;
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_listen should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_LISTENING, sock.state, "sock_listen set sock state to LISTENING");
}

void test_sock_listen_error_in_bind(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set error in bind
    bind_fake.custom_fake = bind_with_error_fake;

    // call function
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_listen should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_listen should set errno on error");
}

void test_sock_listen_error_in_listen(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set error in listen
    listen_fake.custom_fake = listen_with_error_fake;

    // call function
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_listen should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_listen should set errno on error");
}


/**************************************************************************
 * sock_accept tests
 *************************************************************************/

void test_sock_accept_ok_client_found(void)
{
    // create socket
    sock_t sock_s;

    socket_fake.return_val = 123;
    sock_create(&sock_s);

    // set all return values to OK
    listen_fake.return_val = 0;
    sock_listen(&sock_s, 8888);
    accept_fake.return_val = 0;
    sock_t sock_c;

    // call the function
    int res = sock_accept(&sock_s, &sock_c);

    TEST_ASSERT_EQUAL_MESSAGE(1, accept_fake.call_count, "accept() should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(1, res, "sock_accept should return 1 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_LISTENING, sock_s.state, "sock_accept should maintain server socket state");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CONNECTED, sock_c.state, "sock_accept set client state to CONNECTED");
}

void test_sock_accept_ok_client_not_found(void)
{
    // create socket
    sock_t sock_s;

    socket_fake.return_val = 123;
    sock_create(&sock_s);

    // set all return values to OK
    listen_fake.return_val = 0;
    sock_listen(&sock_s, 8888);
    accept_fake.return_val = -1;
    errno = EWOULDBLOCK;
    sock_t sock_c = {0, 0};

    // call the function
    int res = sock_accept(&sock_s, &sock_c);

    TEST_ASSERT_EQUAL_MESSAGE(1, accept_fake.call_count, "accept() should be called once");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_accept should return 0 if errno is EWOULDBLOCK");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_LISTENING, sock_s.state, "sock_accept should maintain server socket state");
    TEST_ASSERT_EQUAL_MESSAGE(0, sock_c.state, "sock_accept should maintain client state");
}

void test_sock_accept_unitialized_socket(void)
{
    sock_t sock;

    // call the function with unitialized sock
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_accept should set errno on error");
}

void test_sock_accept_without_listen(void)
{
    // create socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // call sock_accept without listen
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept on unbound socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_accept should set errno on error");
}

void test_sock_accept_null_client(void)
{
    // create socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // call listen
    listen_fake.return_val = 0;
    sock_listen(&sock, 8888);

    // call accept with null client
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_EQUAL_MESSAGE(1, res, "sock_accept with null client return ok");
}

void test_sock_accept_with_accept_error(void)
{
    // create socket
    sock_t sock_s;

    socket_fake.return_val = 123;
    sock_create(&sock_s);

    // set all return values to OK
    listen_fake.return_val = 0;
    sock_listen(&sock_s, 8888);
    accept_fake.return_val = 0;
    sock_t sock_c;

    // call the function
    accept_fake.custom_fake = accept_with_error_fake;
    int res = sock_accept(&sock_s, &sock_c);

    TEST_ASSERT_EQUAL_MESSAGE(1, accept_fake.call_count, "accept() should be called once");
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept should return error value if the call to accept fails");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_accept should set errno on error");
}


/**************************************************************************
 * sock_connect tests
 *************************************************************************/

void test_sock_connect_ok(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set all return values to OK
    connect_fake.return_val = 0;

    // call the function
    int res = sock_connect(&sock, "::1", 0);

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_connect should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CONNECTED, sock.state, "sock_connect set sock state to CONNECTED");
}

void test_sock_connect_null_client(void)
{
    // call the function with NULL client
    int res = sock_connect(NULL, "::1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect on null socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_unitialized_client(void)
{
    sock_t sock;

    // call the function with uninitialized client
    int res = sock_connect(&sock, "::1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_null_address(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // call the funciton with a null address
    int res = sock_connect(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept a null address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_ipv4_address(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // call the funciton with an unsupported address
    int res = sock_connect(&sock, "127.0.0.1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept ipv4 addresses");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_bad_address(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // call the funciton with a bad address
    int res = sock_connect(&sock, "bad_address", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should fail on bad address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_with_connection_error(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set error in connect()
    connect_fake.custom_fake = connect_with_error_fake;

    // call the funciton with a bad address
    int res = sock_connect(&sock, "::1", 8888);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should fail on connection error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}


/**************************************************************************
 * sock_read tests
 *************************************************************************/

void test_sock_read_ok(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    uint8_t buf[64];
    recv_fake.custom_fake = recv_ok_fake;
    int res = sock_read(&sock, buf, 64);

    TEST_ASSERT_EQUAL_MESSAGE(12, res, "sock_read should return 12 bytes");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("HELLO WORLD", buf, "sock_read should write 'HELLO WORLD' to buf");
}

void test_sock_read_ok_zero_bytes(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    uint8_t buf[64];
    recv_fake.custom_fake = recv_zero_bytes_fake;
    int res = sock_read(&sock, buf, 64);

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_read should return 0 bytes");
}


void test_sock_read_ok_zero_bytes_would_block(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    uint8_t buf[64];
    recv_fake.return_val = -1;
    errno = EWOULDBLOCK;
    int res = sock_read(&sock, buf, 64);

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_read should return 0 when socket would block");
}


void test_sock_read_null_socket(void)
{
    uint8_t buf[64];
    int res = sock_read(NULL, buf, 64);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when reading from NULL socket");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "sock_read should set 'invalid parameter' error number");
}

void test_sock_read_unconnected_socket(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // read without performing sock_connect()
    uint8_t buf[64];
    int res = sock_read(&sock, buf, 64);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when reading from unconnected socket");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "sock_read should set 'invalid parameter' error number");
}

void test_sock_read_null_buffer(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // read to a null buffer
    int res = sock_read(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when passing a null buffer");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "sock_read should set 'invalid parameter' error number");
}


/**************************************************************************
 * sock_write tests
 *************************************************************************/

void test_sock_write_ok(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // write to socket
    send_fake.custom_fake = send_ok_fake;
    int res = sock_write(&sock, (uint8_t *)"HELLO WORLD", 12);

    TEST_ASSERT_GREATER_THAN_MESSAGE(0, send_fake.call_count, "write should be called at least once");
    TEST_ASSERT_EQUAL_MESSAGE(12, res, "sock_write should write 12 bytes");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("HELLO WORLD", global_write_buffer, "sock_write should write 'HELLO WORLD' to socket");
}

void test_sock_write_null_socket(void)
{
    int res = sock_write(NULL,  (uint8_t *)"HELLO WORLD", 12);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when a NULL socket is given");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_write_null_buffer(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // call write with null buffer
    int res = sock_write(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when buffer is NULL");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_write_unconnected_socket(void)
{
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);
    int res = sock_write(&sock, (uint8_t *)"Socket says: hello world", 24);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_write_zero_bytes(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // write to socket
    send_fake.custom_fake = send_ok_fake;
    int res = sock_write(&sock,  (uint8_t *)"HELLO WORLD", 0);

    TEST_ASSERT_EQUAL_MESSAGE(1, send_fake.call_count, "send should be called when writing zero bytes");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_write should write 0 bytes");
}

void test_sock_write_with_error(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // write to socket
    send_fake.custom_fake = send_with_error_fake;
    int res = sock_write(&sock,  (uint8_t *)"HELLO WORLD", 12);

    TEST_ASSERT_GREATER_THAN_MESSAGE(0, send_fake.call_count, "write should be called at least once");
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when write returns an error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}


/**************************************************************************
 * sock_destroy tests
 *************************************************************************/

void test_sock_destroy_ok(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // call destroy on connected socket
    close_fake.return_val = 0;
    int res = sock_destroy(&sock);

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_destroy should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(1, close_fake.call_count, "close should be called only once");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CLOSED, sock.state, "sock_destroy set sock state to CLOSED");
    TEST_ASSERT_EQUAL_MESSAGE(-1, sock.socket, "sock_destroy should reset file descriptor");
}

void test_sock_destroy_null_sock(void)
{
    int res = sock_destroy(NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destroy should fail when socket is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(0, close_fake.call_count, "close should never be called");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_destroy should set errno on error");
}

void test_sock_destroy_closed_sock(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // Destroy socket
    close_fake.return_val = 0;
    sock_destroy(&sock);

    // Call destroy again
    int res = sock_destroy(&sock);

    TEST_ASSERT_EQUAL_MESSAGE(1, close_fake.call_count, "close should be called only once");
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destroy should fail when socket is CLOSED");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_destroy should set errno on error");
}

void test_sock_destroy_with_close_error(void)
{
    // initialize socket
    sock_t sock;

    socket_fake.return_val = 123;
    sock_create(&sock);

    // set socket to connected state
    connect_fake.return_val = 0;
    sock_connect(&sock, "::1", 8888);

    // call destroy on connected socket
    close_fake.custom_fake = close_with_error_fake;
    int res = sock_destroy(&sock);

    TEST_ASSERT_EQUAL_MESSAGE(1, close_fake.call_count, "close should be called only once");
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destroy should fail when close() fails");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_destroy should set errno on error");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CONNECTED, "sock_destroy maintain sock state when close() fails");
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // sock_create tests
    UNIT_TEST(test_sock_create_ok);
    UNIT_TEST(test_sock_create_fail_to_create_socket);

    // sock_listen tests
    UNIT_TEST(test_sock_listen_ok);
    UNIT_TEST(test_sock_listen_unitialized_socket);
    UNIT_TEST(test_sock_listen_error_in_bind);
    UNIT_TEST(test_sock_listen_error_in_listen);
    UNIT_TEST(test_sock_listen_null_socket);

    // sock_accept tests
    UNIT_TEST(test_sock_accept_ok_client_found);
    UNIT_TEST(test_sock_accept_ok_client_not_found);    
    UNIT_TEST(test_sock_accept_unitialized_socket);
    UNIT_TEST(test_sock_accept_without_listen);
    UNIT_TEST(test_sock_accept_null_client);
    UNIT_TEST(test_sock_accept_with_accept_error);

    // sock_connect tests
    UNIT_TEST(test_sock_connect_ok);
    UNIT_TEST(test_sock_connect_null_client);
    UNIT_TEST(test_sock_connect_unitialized_client);
    UNIT_TEST(test_sock_connect_null_address);
    UNIT_TEST(test_sock_connect_ipv4_address);
    UNIT_TEST(test_sock_connect_bad_address);
    UNIT_TEST(test_sock_connect_with_connection_error);

    // sock_read tests
    UNIT_TEST(test_sock_read_ok);
    UNIT_TEST(test_sock_read_ok_zero_bytes);
    UNIT_TEST(test_sock_read_null_socket);
    UNIT_TEST(test_sock_read_unconnected_socket);
    UNIT_TEST(test_sock_read_null_buffer);
    UNIT_TEST(test_sock_read_ok_zero_bytes_would_block);

    // sock_write tests
    UNIT_TEST(test_sock_write_ok);
    UNIT_TEST(test_sock_write_null_socket);
    UNIT_TEST(test_sock_write_null_buffer);
    UNIT_TEST(test_sock_write_unconnected_socket);
    UNIT_TEST(test_sock_write_zero_bytes);
    UNIT_TEST(test_sock_write_with_error);

    // sock_destroy tests
    UNIT_TEST(test_sock_destroy_ok);
    UNIT_TEST(test_sock_destroy_null_sock);
    UNIT_TEST(test_sock_destroy_closed_sock);
    UNIT_TEST(test_sock_destroy_with_close_error);

    return UNIT_TESTS_END();
}
