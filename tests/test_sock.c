#include <errno.h>

#include "unity.h"
#include "sock.h"

void test_sock_create(void)
{
    sock_t sock;
    sock_create(&sock);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_OPENED, "sock_create should leave sock in 'OPENED' state");

    // Close socket
    sock_destroy(&sock);
}

void test_sock_listen_unitialized_socket(void) {
    sock_t sock;
    int res = sock_listen(&sock, 8888);

    TEST_ASSERT_LESS_THAN_MESSAGE(res, 0, "sock_listen on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_listen(void) {
    sock_t sock;
    
    if (sock_create(&sock) < 0) return; // an issue independent of the test ocurred
    
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_listen should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_LISTENING, "sock_listen set sock state to LISTENING");
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
    UNITY_BEGIN();
    RUN_TEST(test_sock_create);
    RUN_TEST(test_sock_listen_unitialized_socket);
    RUN_TEST(test_sock_listen);
    RUN_TEST(test_sock_accept_unitialized_socket);
    RUN_TEST(test_sock_accept_unbound_socket);
    RUN_TEST(test_sock_accept_null_client);
    RUN_TEST(test_sock_connect_unitialized_client);
    RUN_TEST(test_sock_connect_null_address);
    RUN_TEST(test_sock_connect_ipv4_address);
    RUN_TEST(test_sock_connect_bad_address);
    RUN_TEST(test_sock_read_unconnected_socket);
    RUN_TEST(test_sock_write_unconnected_socket);
    return UNITY_END();
}
