#include "unit.h"
#include "http_methods.h"
#include "fff.h"


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_create, sock_t *);
FAKE_VALUE_FUNC(int, sock_listen, sock_t *, uint16_t);
FAKE_VALUE_FUNC(int, sock_accept, sock_t *, sock_t *);
FAKE_VALUE_FUNC(int, sock_connect, sock_t *, char *, uint16_t);
FAKE_VALUE_FUNC(int, sock_read, sock_t *, char *, int, int);
FAKE_VALUE_FUNC(int, sock_write, sock_t *, char *, int);
FAKE_VALUE_FUNC(int, sock_destroy, sock_t *);
FAKE_VALUE_FUNC(int, client_init_connection, h2states_t *);
FAKE_VALUE_FUNC(int, server_init_connection, h2states_t *);
FAKE_VOID_FUNC(puts, char *);


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
  FAKE(puts)                            \


/**
* Socket typedef
*/
typedef struct {
   int fd;
   enum {
       SOCK_CLOSED,
       SOCK_OPENED,
       SOCK_LISTENING,
       SOCK_CONNECTED
   } state;
} sock_t;


void setup(){
  /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
}


void sock_create_custom(sock_t * s){
  s.fd=2;
  s.state=SOCK_OPENED;
}
void sock_listen_custom(sock_t * s, uint16_t n){ s.state=SOCK_LISTENING;}
void sock_accept_custom(sock_t * ss, sock_t * sc){
  sc.fd=2;
  sc.state=SOCK_CONNECTED;
}
void sock_connect_custom(sock_t * s, char * ip, uint16_t p){ s.state=SOCK_CONNECTED;}
void sock_read_custom(char * buf){ * buf = "hola";}
void sock_destroy_custom(sock_t * s){ s.state=SOCK_CLOSED;}


void test_http_client_connect_success(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=0;
  void (*sock_listen_custom_fake[])(char *) = {sock_listen_custom};
  SET_CUSTOM_FAKE_SEQ(sock_listen, sock_listen_custom_fake, 1);
  sock_accept.return_val=0;
  void (*sock_accept_custom_fake[])(char *) = {sock_accept_custom};
  SET_CUSTOM_FAKE_SEQ(sock_accept, sock_accept_custom_fake, 1);
  server_init_connection.return_val=0;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)server_init_connection);

  TEST_ASSERT_EQUAL(is,0);
}

void test_http_client_connect_fail_v1(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=0;
  void (*sock_listen_custom_fake[])(char *) = {sock_listen_custom};
  SET_CUSTOM_FAKE_SEQ(sock_listen, sock_listen_custom_fake, 1);
  sock_accept.return_val=0;
  void (*sock_accept_custom_fake[])(char *) = {sock_accept_custom};
  SET_CUSTOM_FAKE_SEQ(sock_accept, sock_accept_custom_fake, 1);
  server_init_connection.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)server_init_connection);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Problems sending server data");

  TEST_ASSERT_EQUAL(is,-1);
}


void test_http_client_connect_fail_v2(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=0;
  void (*sock_listen_custom_fake[])(char *) = {sock_listen_custom};
  SET_CUSTOM_FAKE_SEQ(sock_listen, sock_listen_custom_fake, 1);
  sock_accept.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Not client found");

  TEST_ASSERT_EQUAL(is,-1);
}


void test_http_client_connect_fail_v3(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Partial error in server creation");

  TEST_ASSERT_EQUAL(is,-1);
}

void test_http_client_connect_fail_v4(void){
  setup();

  sock_create.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Server could not be created");

  TEST_ASSERT_EQUAL(is,-1);
}

void test_http_client_connect_success(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=0;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)client_init_connection);

  TEST_ASSERT_EQUAL(cc,0);
}

void test_http_client_connect_fail_client_init_connection(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)client_init_connection);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Problems sending client data");

  TEST_ASSERT_EQUAL(cc,-1);
}

void test_http_client_connect_fail_sock_connect(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Client could not be created");

  TEST_ASSERT_EQUAL(cc,-1);
}

void test_http_client_connect_fail_sock_create(void){
  setup();

  sock_create.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Client could not be created");

  TEST_ASSERT_EQUAL(cc,-1);
}

void test_http_disconnect_success_v1(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=-1;

  http_client_connect(12,"::");


  sock_destroy.return_val=0;
  void (*custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_destroy, custom_fake, 1);

  int d=http_client_disconnect();


  TEST_ASSERT_EQUAL(d,0);

  TEST_ASSERT_EQUAL(client_sock.state,SOCK_CLOSED);
}

void test_http_disconnect_success_v2(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=0;

  http_client_connect(12,"::");


  sock_destroy.return_val=0;
  void (*custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_destroy, custom_fake, 1);

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_destroy);

  TEST_ASSERT_EQUAL(d,0);

  TEST_ASSERT_EQUAL(client_sock.state,SOCK_CLOSED);
}

void test_http_disconnect_fail(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=0;

  http_client_connect(12,"::");


  sock_destroy.return_val=-1;

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_destroy);

  TEST_ASSERT_EQUAL(d,-1);

  TEST_ASSERT_EQUAL(client_sock.state,SOCK_OPENED);

  TEST_ASSERT_EQUAL_MESSAGE("Client could not be disconnected");
}

void test_http_write_success(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=0;
  void (*sock_listen_custom_fake[])(char *) = {sock_listen_custom};
  SET_CUSTOM_FAKE_SEQ(sock_listen, sock_listen_custom_fake, 1);
  sock_accept.return_val=0;
  void (*sock_accept_custom_fake[])(char *) = {sock_accept_custom};
  SET_CUSTOM_FAKE_SEQ(sock_accept, sock_accept_custom_fake, 1);
  server_init_connection.return_val=0;

  http_init_server(12);

  sock_read.return_val=4;

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_write);

  TEST_ASSERT_EQUAL(wr,4);
}

void test_http_write_two_calls(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=0;

  http_client_connect(12,"::");


  sock_read.return_val=0;

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_write(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_write);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Could not read");

  TEST_ASSERT_EQUAL(wr,0);
}

void test_http_write_fail(void){
  setup();

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_write(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Could not write");

  TEST_ASSERT_EQUAL(wr,-1);
}

void test_http_read_success(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_listen.return_val=0;
  void (*sock_listen_custom_fake[])(char *) = {sock_listen_custom};
  SET_CUSTOM_FAKE_SEQ(sock_listen, sock_listen_custom_fake, 1);
  sock_accept.return_val=0;
  void (*sock_accept_custom_fake[])(char *) = {sock_accept_custom};
  SET_CUSTOM_FAKE_SEQ(sock_accept, sock_accept_custom_fake, 1);
  server_init_connection.return_val=0;

  http_init_server(12);

  sock_read.return_val=4;
  void (*custom_fake[])(char *) = {sock_read_custom};
  SET_CUSTOM_FAKE_SEQ(sock_read, custom_fake, 1);

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_read);

  TEST_ASSERT_EQUAL(buf, "hola");

  TEST_ASSERT_EQUAL(rd,4);
}

void test_http_read_two_calls(void){
  setup();

  sock_create.return_val=0;
  void (*sock_create_custom_fake[])(char *) = {sock_destroy_custom};
  SET_CUSTOM_FAKE_SEQ(sock_create, sock_create_custom_fake, 1);
  sock_connect.return_val=0;
  void (*sock_connect_custom_fake[])(char *) = {sock_connect_custom};
  SET_CUSTOM_FAKE_SEQ(sock_connect, sock_connect_custom_fake, 1);
  client_init_connection.return_val=0;

  http_client_connect(12,"::");


  sock_read.return_val=0;

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);


  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_read);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Could not read");

  TEST_ASSERT_EQUAL(rd,0);
}

void test_http_read_fail(void){
  setup();

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE("Could not read");

  TEST_ASSERT_EQUAL(rd,-1);
}

int main(void){
  UNITY_BEGIN();

  RUN_TEST(test_http_write_success);
  RUN_TEST(test_http_read_success);
  RUN_TEST(test_http_write_two_calls);
  RUN_TEST(test_http_read_two_calls);
  RUN_TEST(test_http_write_fail);
  RUN_TEST(test_http_read_fail);

  return UNITY_END();
}
