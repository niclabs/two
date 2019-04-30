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
FAKE_VALUE_FUNC(int, connect, int, const struct sockaddr *, socklen_t);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) \
    FAKE(socket)             \
    FAKE(bind)               \
    FAKE(listen)             \
    FAKE(accept)             \
    FAKE(read)               \
    FAKE(write)              \
    FAKE(close)              \
    FAKE(connect)


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
    
    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_OPENED, "sock_create should leave sock in 'OPENED' state");
    TEST_ASSERT_EQUAL_MESSAGE(sock.fd, 123, "sock_create should set file descriptor in sock structure");

    // Close socket
    sock_destroy(&sock);
}

void test_sock_create_null_sock(void){
    socket_fake.return_val=-1;
    int res=sock_create(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_create should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_create should set errno on error");

}

void test_sock_create_fail_to_create_socket(void)
{
    sock_t sock;

    // Set error return for socket()
    socket_fake.return_val = -1;
    int res = sock_create(&sock);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_create should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_create should set errno on error");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CLOSED, "sock_create should leave sock in 'CLOSED' state on error");
}

void test_sock_listen_unitialized_socket(void) {
    sock_t sock;
    int res = sock_listen(&sock, 8888);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_listen on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_listen_null_socket(void){
    int res=sock_listen(NULL, 8888);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_listen on NULL socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_listen(void) {
    sock_t sock;
    
    // Set success return for socket() and listen()
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    listen_fake.return_val = 0;
    int res = sock_listen(&sock, 8888);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_listen should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_LISTENING, "sock_listen set sock state to LISTENING");
}

void test_sock_listen_bad_port(void){
    sock_t sock;
    socket_fake.return_val=123;
    sock_create(&sock);

    listen_fake.return_val=-1;

    int res= sock_listen(&sock, -1);
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_listen should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_listen_error_return(void) {
    sock_t sock;
    
    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    // Set error return value for listen()
    listen_fake.return_val = -1;
    errno = ENOTSOCK;

    int res = sock_listen(&sock, 8888);
    
    // Sock create should put the socket in opened state
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_listen should return -1 on error");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_listen should set errno on error");
}

void test_sock_accept(void){
    sock_t sock_s, sock_c;
    socket_fake.return_val=123;
    sock_create(&sock_s);
    sock_create(&sock_c);
    
    listen_fake.return_val = 0;
    sock_listen(&sock_s, 8888);

    connect_fake.return_val=0;
    sock_connect(&sock_c, "::1", 8888);

    accept_fake.return_val=0;
    int res = sock_accept(&sock_s, &sock_c);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_accept should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock_s.state, SOCK_CONNECTED, "sock_accept set server state to CONNECTED");
    TEST_ASSERT_EQUAL_MESSAGE(sock_c.state, SOCK_CONNECTED, "sock_accept set client state to CONNECTED");
}

void test_sock_accept_unitialized_socket(void) {
    sock_t sock;
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_accept_unbound_socket(void) {
    sock_t sock;

    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);

    // Call sock_accept without sock_listen first
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept on unbound socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_accept_null_client(void) {
    sock_t sock;

    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);

    // Set succesful return value for listen()
    listen_fake.return_val = 0;
    sock_listen(&sock, 8888);

    // Call accept with null client
    int res = sock_accept(&sock, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept with null client should return error value"); 
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");
}

void test_sock_accept_not_connected_client(){
    sock_t sock_server, sock_client;
    socket_fake.return_val=123;
    sock_create(&sock_server);
    sock_create(&sock_client);

    listen_fake.return_val=0;
    sock_listen(&sock_server, 8888);
    int res=sock_accept(&sock_server,&sock_client);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_accept with not connected client should return error value"); 
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_accept should set errno on error");

}

void test_sock_connect(void){
    sock_t sock;
    // Set success return for socket() and connect()
    socket_fake.return_val = 123;
    sock_create(&sock);
    connect_fake.return_val=0;

    int res = sock_connect(&sock, "::1", 0);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_connect should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CONNECTED, "sock_connect set sock state to CONNECTED");
}

void test_sock_connect_unitialized_client(void) {
    sock_t sock;
    int res = sock_connect(&sock, "::1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_null_address(void) {
    sock_t sock;

    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);    

    // Call connect with NULL address
    int res = sock_connect(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept a null address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_ipv4_address(void) {
    sock_t sock;

    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    int res = sock_connect(&sock, "127.0.0.1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept ipv4 addresses");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_connect_bad_address(void) {
    sock_t sock;
    
    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);

    int res = sock_connect(&sock, "bad_address", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should fail on bad address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_connect should set errno on error");
}

void test_sock_read_unconnected_socket(void) {
    sock_t sock;
    char buf[256];
    
    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);
    
    int res = sock_read(&sock, buf, 256, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_read should set errno on error");
}

void test_sock_write_unconnected_socket(void) {
    sock_t sock;
    char buf[256];
    
    // Set success return for socket()
    socket_fake.return_val = 123;
    sock_create(&sock);

    int res = sock_write(&sock, buf, 256);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_write should set errno on error");
}

void test_sock_destroy(void){
    sock_t sock;
    socket_fake.return_val=123;
    sock_create(&sock);
    close_fake.return_val=0;
    int res=sock_destroy(&sock);
    TEST_ASSERT_EQUAL_MESSAGE(res, 0, "sock_destroy should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(sock.state, SOCK_CLOSED, "sock_destroy set sock state to CLOSED");
}

 void test_sock_destroy_null_sock(void){
    close_fake.return_val=-1;
    int res=sock_destroy(NULL);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destrroy should fail when socket is NULL");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_destroy should set errno on error");
 }

 void test_sock_destroy_closed_sock(void){
     sock_t sock;
     socket_fake.return_val=123;
     sock_create(&sock);
     sock_destroy(&sock);
     int res=sock_destroy(&sock);
     close_fake.return_val=-1;
     TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destrroy should fail when socket is CLOSED");
     TEST_ASSERT_NOT_EQUAL_MESSAGE(errno, 0, "sock_destroy should set errno on error");


 }

// TODO: add more tests for read and write
 
int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_sock_create);
    UNIT_TEST(test_sock_create_fail_to_create_socket);
    UNIT_TEST(test_sock_create_null_sock);
    UNIT_TEST(test_sock_listen_unitialized_socket); 
    UNIT_TEST(test_sock_listen_error_return);
    UNIT_TEST(test_sock_listen_null_socket);
    UNIT_TEST(test_sock_listen_bad_port);
    UNIT_TEST(test_sock_listen);
    UNIT_TEST(test_sock_accept_unitialized_socket); 
    UNIT_TEST(test_sock_accept_unbound_socket); 
    UNIT_TEST(test_sock_accept_null_client);
    UNIT_TEST(test_sock_accept_not_connected_client);
    UNIT_TEST(test_sock_accept);
    UNIT_TEST(test_sock_connect_unitialized_client);
    UNIT_TEST(test_sock_connect_null_address);
    UNIT_TEST(test_sock_connect_ipv4_address); 
    UNIT_TEST(test_sock_connect_bad_address);
    UNIT_TEST(test_sock_connect);
    UNIT_TEST(test_sock_read_unconnected_socket); 
    UNIT_TEST(test_sock_write_unconnected_socket);
    UNIT_TEST(test_sock_destroy);
    UNIT_TEST(test_sock_destroy_null_sock);
    UNIT_TEST(test_sock_destroy_closed_sock);
    return UNIT_TESTS_END();
}
