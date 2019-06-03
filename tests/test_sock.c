#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <unistd.h>
#include <fcntl.h>


#include <sys/types.h>
#include <sys/socket.h>
#include <pthread.h>

#include "unit.h"
#include "sock.h"
#include "fff.h"

/*filename: to simulate user input in test where is needed, user input is redirected 
to content of this file.*/
#define filename "mocking_input.txt"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, socket, int, int, int);
FAKE_VALUE_FUNC(int, bind, int, const struct sockaddr *, socklen_t);
FAKE_VALUE_FUNC(int, listen, int, int);
FAKE_VALUE_FUNC(int, accept, int, struct sockaddr *, socklen_t *);
FAKE_VALUE_FUNC(int, close, int);
FAKE_VALUE_FUNC(int, connect, int, const struct sockaddr *, socklen_t);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) \
    FAKE(socket)             \
    FAKE(bind)               \
    FAKE(listen)             \
    FAKE(accept)             \
    FAKE(close)              \
    FAKE(connect)
 

// TODO: a better way could be to use unity_fixtures
// https://github.com/ThrowTheSwitch/Unity/blob/199b13c099034e9a396be3df9b3b1db1d1e35f20/examples/example_2/test/TestProductionCode.c

/*TODO: separate functions and definitions that are used by test and put in another file.*/

/*Struct used by pthread_create, stores arguments of function used by thread.*/
struct thread_sock
{
    uint16_t port;
};

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

/*Function to mock input from user. The content of the file will act as the input*/
void io_mock(void){

    FILE *file_pointer; 
	
	file_pointer = fopen(filename, "w"); 
 
	fprintf(file_pointer, "Socket says: hello world\n"); //26 chars
	
	fclose(file_pointer); 
}
/*Function to erase file that was created in the mock*/
void erase_io_mock(void){
    remove(filename);
}

//FILE* fdopen(int fd, char* opt);// when not commented get this error on make test: /usr/include/stdio.h:265:14: note: previous declaration of ‘fdopen’ was here: extern FILE *fdopen (int __fd, const char *__modes) __THROW __wur;

/*Run client in thread to test functionalities that need connection established.*/
void *thread_connect(void *arg)
{
    struct thread_sock *client = arg;
    uint16_t port = client->port;
    socket_fake.return_val = 122;
    sock_t socket_c;
    sock_create(&socket_c);
    int res=sock_connect(&socket_c, "::1", port);

    intptr_t result = (intptr_t)res;
    sock_destroy(&socket_c);
    return (void *)result;
}


/**************************************************************************
 * posix socket mocks
 *************************************************************************/
int socket_with_error_fake(int domain, int type, int protocol) {
    errno = EAFNOSUPPORT;
    return -1;
}

int bind_with_error_fake(int sockfd, const struct sockaddr *addr,
                 socklen_t addrlen) {
    errno = ENOTSOCK;
    return -1;
}

int listen_with_error_fake(int sockfd, int backlog) {
    errno = EOPNOTSUPP;
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
    TEST_ASSERT_EQUAL_MESSAGE(123, sock.fd, "sock_create should set file descriptor in sock structure");
    sock_destroy(&sock);
}

void test_sock_create_null_sock(void)
{
    socket_fake.return_val = -1;
    int res = sock_create(NULL);

    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "sock_create should return -1 on NULL pointer parameter");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_create should set errno on error");
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

void test_sock_accept_ok(void)
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

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_accept should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_LISTENING, sock_s.state, "sock_accept should maintain server socket state");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CONNECTED, sock_c.state, "sock_accept set client state to CONNECTED");
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

    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_accept with null client return ok");
}

void test_sock_connect_ok(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    connect_fake.return_val = 0;
    int res = sock_connect(&sock, "::1", 0);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_connect should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CONNECTED, sock.state, "sock_connect set sock state to CONNECTED");
}

void test_sock_connect_null_client(void)
{
    connect_fake.return_val = -1;
    int res = sock_connect(NULL, "::1", 0);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect on null socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_unitialized_client(void)
{
    sock_t sock;
    connect_fake.return_val = -1;
    int res = sock_connect(&sock, "::1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect on unitialized socket should return error value");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_null_address(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    connect_fake.return_val = -1;
    int res = sock_connect(&sock, NULL, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept a null address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_ipv4_address(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    connect_fake.return_val = -1;
    int res = sock_connect(&sock, "127.0.0.1", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should not accept ipv4 addresses");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}

void test_sock_connect_bad_address(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    connect_fake.return_val = -1;
    int res = sock_connect(&sock, "bad_address", 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_connect should fail on bad address");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_connect should set errno on error");
}


void test_sock_read_ok(void){

    char buffer[256];
    socket_fake.return_val = 123;

    struct thread_sock *thread_client;
    thread_client = malloc(sizeof(thread_client));
    thread_client->port = 1111;
    pthread_t client_thread;

    sock_t sock_s;
    sock_create(&sock_s);

    sock_listen(&sock_s, 1111);

    sock_t sock_c2;
    sock_create(&sock_c2);
   
    pthread_create(&client_thread, NULL, thread_connect, thread_client);

    sock_accept(&sock_s, &sock_c2);
    
    /*Due to giving socket the minimun number of fd available, in the test sock_c2 has fd of 0,
    corresponding to stdin.*/
    /*Redirecting reading to read from file created in io_mock instead of stdin.*/
    FILE *file_sock = fdopen(sock_c2.fd, "w+r");
    freopen(filename, "r", file_sock);
   
    int res= sock_read(&sock_c2, buffer,25, 0);

    pthread_join(client_thread, NULL);
    free(thread_client);
    thread_client=NULL;
    
    
    TEST_ASSERT_EQUAL_MESSAGE(25, res, "sock_read should have read 25 bytes");
    TEST_ASSERT_EQUAL_MESSAGE(0, errno, "sock_read should not set errno on success");

}

void test_sock_read_bad_timeout(void) {

    char buffer[256];
    socket_fake.return_val = 11;

    struct thread_sock *thread_client;
    thread_client = malloc(sizeof(thread_client));
    thread_client->port = 1122;
    pthread_t client_thread;

    sock_t sock_s;
    sock_create(&sock_s);

    sock_listen(&sock_s, 1122);

    sock_t sock_c2;
    sock_create(&sock_c2);
   
    pthread_create(&client_thread, NULL, thread_connect, thread_client);

    sock_accept(&sock_s, &sock_c2);
   
    int res= sock_read(&sock_c2, buffer,25, -1);

    pthread_join(client_thread, NULL);
    free(thread_client);
    thread_client=NULL;

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when timeout is not valid");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "sock_read should set errno on error");
}

void test_sock_read_null_buffer(void) {
    socket_fake.return_val = 12;

    struct thread_sock *thread_client;
    thread_client = malloc(sizeof(thread_client));
    thread_client->port = 1122;
    pthread_t client_thread;

    sock_t sock_s;
    sock_create(&sock_s);

    sock_listen(&sock_s, 1122);

    sock_t sock_c2;
    sock_create(&sock_c2);
   
    pthread_create(&client_thread, NULL, thread_connect, thread_client);

    sock_accept(&sock_s, &sock_c2);
   
    int res= sock_read(&sock_c2, NULL,25, 0);

    pthread_join(client_thread, NULL);
    free(thread_client);
    thread_client=NULL;

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when buffer is NULL");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "sock_read should set errno on error");
}

void test_sock_read_null_socket(void){
    char buf[256];
    int res=sock_read(NULL, buf, 255, 0);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when reading from NULL socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_read should set errno on error");
}

void test_sock_read_unconnected_socket(void) {
    sock_t sock;
    char buf[256];
    socket_fake.return_val = 123;
    sock_create(&sock);
    int res = sock_read(&sock, buf, 255, 0);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_read should fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_read should set errno on error");
}

void test_sock_write_ok(void)
{
    char buffer[26]="Socket says: hello world\n";
    socket_fake.return_val = 123;

    struct thread_sock *thread_client;
    thread_client = malloc(sizeof(thread_client));
    thread_client->port = 1111;
    pthread_t client_thread;

    sock_t sock_s;
    sock_create(&sock_s);

    sock_listen(&sock_s, 1111);

    sock_t sock_c2;
    sock_create(&sock_c2);
   
    pthread_create(&client_thread, NULL, thread_connect, thread_client);

    sock_accept(&sock_s, &sock_c2);
    int res= sock_write(&sock_c2,buffer,25);
    free(thread_client);
    thread_client=NULL;
    pthread_join(client_thread, NULL);
    
    TEST_ASSERT_EQUAL_MESSAGE(25, res, "sock_write should have written 25 bytes");
    TEST_ASSERT_EQUAL_MESSAGE(0, errno, "sock_write should not set errno on success");
}

void test_sock_write_null_socket(void){
    int res=sock_write(NULL, "Socket says: hello world", 24);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when reading from NULL socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_write_null_buffer(void) {
    socket_fake.return_val = 123;

    struct thread_sock *thread_client;
    thread_client = malloc(sizeof(thread_client));
    thread_client->port = 1111;
    pthread_t client_thread;

    sock_t sock_s;
    sock_create(&sock_s);

    sock_listen(&sock_s, 1111);

    sock_t sock_c2;
    sock_create(&sock_c2);
   
    pthread_create(&client_thread, NULL, thread_connect, thread_client);

    sock_accept(&sock_s, &sock_c2);
    int res= sock_write(&sock_c2,NULL,24);
    free(thread_client);
    thread_client=NULL;
    pthread_join(client_thread, NULL);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when buffer is NULL");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_write_unconnected_socket(void) {
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    int res = sock_write(&sock, "Socket says: hello world", 24);

    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_write should fail when reading from unconnected socket");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_write should set errno on error");
}

void test_sock_destroy_ok(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    close_fake.return_val = 0;
    int res = sock_destroy(&sock);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "sock_destroy should return 0 on success");
    TEST_ASSERT_EQUAL_MESSAGE(SOCK_CLOSED, sock.state, "sock_destroy set sock state to CLOSED");
}

void test_sock_destroy_null_sock(void)
{
    close_fake.return_val = -1;
    int res = sock_destroy(NULL);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destrroy should fail when socket is NULL");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_destroy should set errno on error");
}

void test_sock_destroy_closed_sock(void)
{
    sock_t sock;
    socket_fake.return_val = 123;
    sock_create(&sock);
    sock_destroy(&sock);
    close_fake.return_val = -1;
    int res = sock_destroy(&sock);
    TEST_ASSERT_LESS_THAN_MESSAGE(0, res, "sock_destrroy should fail when socket is CLOSED");
    TEST_ASSERT_NOT_EQUAL_MESSAGE(0, errno, "sock_destroy should set errno on error");
}

int main(void)
{
    /*CREATE FILE TO MOCK USER INPUT*/
    io_mock();
    UNIT_TESTS_BEGIN();
    /*TEST OF FUNCTIONS IN CASE OF GOOD USE*/
    UNIT_TEST(test_sock_create_ok);
    UNIT_TEST(test_sock_listen_ok);
    UNIT_TEST(test_sock_accept_ok);
    UNIT_TEST(test_sock_connect_ok);
    UNIT_TEST(test_sock_write_ok);
    UNIT_TEST(test_sock_read_ok);
    UNIT_TEST(test_sock_destroy_ok);
    /*TEST OF FUNCTIONS IN CASE OF BAD USE*/
    UNIT_TEST(test_sock_create_fail_to_create_socket);
    UNIT_TEST(test_sock_create_null_sock);
    UNIT_TEST(test_sock_listen_unitialized_socket);
    UNIT_TEST(test_sock_listen_error_in_bind);
    UNIT_TEST(test_sock_listen_error_in_listen);
    UNIT_TEST(test_sock_listen_null_socket);
    UNIT_TEST(test_sock_accept_unitialized_socket);
    UNIT_TEST(test_sock_accept_without_listen);
    UNIT_TEST(test_sock_accept_null_client);
    UNIT_TEST(test_sock_connect_null_client);
    UNIT_TEST(test_sock_connect_unitialized_client);
    UNIT_TEST(test_sock_connect_null_address);
    UNIT_TEST(test_sock_connect_ipv4_address);
    UNIT_TEST(test_sock_connect_bad_address);
    UNIT_TEST(test_sock_read_null_socket);
    UNIT_TEST(test_sock_read_null_buffer);
    UNIT_TEST(test_sock_read_bad_timeout);
    UNIT_TEST(test_sock_read_unconnected_socket);
    UNIT_TEST(test_sock_write_null_socket);
    UNIT_TEST(test_sock_write_null_buffer);
    UNIT_TEST(test_sock_write_unconnected_socket);
    UNIT_TEST(test_sock_destroy_null_sock);
    UNIT_TEST(test_sock_destroy_closed_sock);
    /*ERASE FILE TO MOCK USER INPUT*/
    erase_io_mock();
    return UNIT_TESTS_END();
}
