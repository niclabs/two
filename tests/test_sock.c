#include <errno.h>

#include "unit.h"
#include "sock.h"
#include "fff.h"

#include <sys/types.h>
#include <sys/socket.h>

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, bind, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, read, int, void *, size_t *);
FAKE_VALUE_FUNC(int, write, int, void *, size_t *);
FAKE_VALUE_FUNC(int, close, int);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) \
    FAKE(socket)             \
    FAKE(bind)               \
    FAKE(listen)             \
    FAKE(accept)             \
    FAKE(read)               \
    FAKE(write)              \
    FAKE(close)

// TODO: a better way could be to use unity_fixtures 
// https://github.com/ThrowTheSwitch/Unity/blob/199b13c099034e9a396be3df9b3b1db1d1e35f20/examples/example_2/test/TestProductionCode.c
void setUp(void) {
    /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
}

void test_sock_create(void)
{
    sock_t sock;
    
    // Set return value for socket
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_OPENED, "sock_create should leave sock in 'OPENED' state");
    TEST_ASSERT_EQUAL_MESSAGE(sock.fd, 123, "sock_create should set file descriptor in sock structure");

    // Close socket
    sock_destroy(&sock);
}

void test_sock_create_fail_to_create_socket(void)
{
    sock_t sock;

    // Set return value for socket
    socket_fake.return_val = -1;
    int res = sock_create(&sock);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_create should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CLOSED, "sock_create should leave sock in 'CLOSED' state on error");
}

void test_sock_listen_unitialized_socket(void) {
    sock_t sock;
    int res = sock_listen(&sock, 8888);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_listen on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_listen(void) {
    sock_t sock;
    
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    listen_fake.return_val = 0;
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_listen should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_LISTENING, "sock_listen set sock state to LISTENING");
}

void test_sock_listen_error_return(void) {
    sock_t sock;
    
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    listen_fake.return_val = -1;
    errno = ENOTSOCK;
    int res = sock_listen(&sock, 8888);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_listen should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CLOSED, "sock_listen should leave sock in 'CLOSED' state on error");
}

void test_sock_accept_unitialized_socket(void) {
    sock_t sock;
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_accept on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_accept_unbound_socket(void) {
    sock_t sock;
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    if (sock_listen(&sock, 8888) < 0) return; // an issue independent of the test ocurred
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_accept on unbound socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_accept_null_client(void) {
    sock_t sock;
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    if (sock_listen(&sock, 8888) < 0) return; // an issue independent of the test ocurred

    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_accept with null client should return error value"); // TODO: should it?
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_connect_unitialized_client(void) {
    sock_t sock;
    int res = sock_connect(&sock, "::1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_connect on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_null_address(void) {
    sock_t sock;
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    int res = sock_connect(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_connect should not accept a null address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_ipv4_address(void) {
    sock_t sock;
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    int res = sock_connect(&sock, "127.0.0.1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_connect should not accept ipv4 addresses");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_bad_address(void) {
    sock_t sock;
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    int res = sock_connect(&sock, "bad_address", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_connect should fail on bad address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_read_unconnected_socket(void) {
    sock_t sock;
    char buf[256];
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    int res = sock_read(&sock, buf, 256, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_read should not fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_read should set errno on error");
}

void test_sock_write_unconnected_socket(void) {
    sock_t sock;
    char buf[256];
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    int res = sock_write(&sock, buf, 256);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_write should not fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_write should set errno on error");
}
 
int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_sock_create);
    UNIT_TEST(test_sock_create_fail_to_create_socket);
    UNIT_TEST(test_sock_listen_unitialized_socket);
    UNIT_TEST(test_sock_listen);
    UNIT_TEST(test_sock_accept_unitialized_socket);
    UNIT_TEST(test_sock_accept_unbound_socket);
    UNIT_TEST(test_sock_accept_null_client);
    UNIT_TEST(test_sock_connect_unitialized_client);
    UNIT_TEST(test_sock_connect_null_address);
    UNIT_TEST(test_sock_connect_ipv4_address);
    UNIT_TEST(test_sock_connect_bad_address);
    UNIT_TEST(test_sock_read_unconnected_socket);
    UNIT_TEST(test_sock_write_unconnected_socket);
    return UNIT_TESTS_END();
}
