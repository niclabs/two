#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http_methods.h"
#include "http2api.h"

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
FAKE_VALUE_FUNC(int, client_init_connection, h2states_t *);
FAKE_VALUE_FUNC(int, server_init_connection, h2states_t *);


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


void setup(){
  /* Register resets */
  FFF_FAKES_LIST(RESET_FAKE);

  /* reset common FFF internal structures */
  FFF_RESET_HISTORY();
}


int sock_create_custom_fake(sock_t * s){
  s->fd=2;
  s->state=SOCK_OPENED;
  return 0;
}
int sock_listen_custom_fake(sock_t * s, uint16_t n){
  s->state=SOCK_LISTENING;
  return 0;
}
int sock_accept_custom_fake(sock_t * ss, sock_t * sc){
  sc->fd=2;
  sc->state=SOCK_CONNECTED;
  return 0;
}
int sock_connect_custom_fake(sock_t * s, char * ip, uint16_t p){
  s->state=SOCK_CONNECTED;
  return 0;
}
int sock_read_custom_fake(sock_t * s, char * buf, int l, int u){
  buf[0] = 'h';
  buf[1] = 'o';
  buf[2] = 'l';
  buf[3] = 'a';
  (void) s;
  (void) u;
  return l;
}
int sock_destroy_custom_fake(sock_t * s){
  s->state=SOCK_CLOSED;
  return 0;
}


void test_http_init_server_success(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=0;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=0;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)server_init_connection);

  TEST_ASSERT_EQUAL(is,0);
}


void test_http_init_server_fail_server_init_connection(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=0;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=0;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)server_init_connection);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(is,-1,"Problems sending server data");
}


void test_http_init_server_fail_sock_accept(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=0;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)sock_accept);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(is,-1,"Not client found");
}


void test_http_init_server_fail_sock_listen(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_listen);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(is,-1,"Partial error in server creation");
}


void test_http_init_server_fail_sock_create(void){
  setup();

  sock_create_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(is,-1,"Server could not be created");
}


void test_http_client_connect_success(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)client_init_connection);

  TEST_ASSERT_EQUAL(cc,0);
}


void test_http_client_connect_fail_client_init_connection(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)client_init_connection);
  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(cc,-1,"Problems sending client data");
}


void test_http_client_connect_fail_sock_connect(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)sock_connect);
  TEST_ASSERT_EQUAL(fff.call_history[2], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(cc,-1,"Error on client connection");
}


void test_http_client_connect_fail_sock_create(void){
  setup();

  sock_create_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_create);
  TEST_ASSERT_EQUAL(fff.call_history[1], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(cc,-1,"Error on client creation");
}


void test_http_disconnect_success_v1(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  http_client_connect(12,"::");

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(d,0);
}


void test_http_disconnect_success_v2(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_destroy_fake.return_val=0;
  sock_destroy_fake.custom_fake=sock_destroy_custom_fake;

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_destroy);

  TEST_ASSERT_EQUAL(d,0);
}


void test_http_disconnect_fail(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_destroy_fake.return_val=-1;

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_destroy);

  TEST_ASSERT_EQUAL_MESSAGE(d,-1,"Client could not be disconnected");
}


void test_http_write_success(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=0;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=0;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;;

  http_init_server(12);


  int sock_write_r[1] = { 4 };
  SET_RETURN_SEQ(sock_write, sock_write_r, 1);

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_write);

  TEST_ASSERT_EQUAL(wr,4);
}


void test_http_write_two_calls(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  int sock_write_r[1] = { 0 };
  SET_RETURN_SEQ(sock_write, sock_write_r, 1);

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_write(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_write);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(wr,0,"Could not read");
}


void test_http_write_fail(void){
  setup();

  char * buf = (char*)malloc(256);
  buf="hola";
  int wr = http_write(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(wr,-1,"Could not write");
}


void test_http_read_success(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=0;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=0;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;

  http_init_server(12);


  sock_read_fake.custom_fake=sock_read_custom_fake;
  sock_read_fake.return_val=4;

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)sock_read);

  TEST_ASSERT_EQUAL(buf, "hola");

  TEST_ASSERT_EQUAL(rd,4);
}


void test_http_read_fail_two_calls(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=0;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_read_fake.return_val=0;

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);


  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_read);
  TEST_ASSERT_EQUAL(fff.call_history[4], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(rd,0,"Could not read");
}

void test_http_read_fail_not_connected_client(void){
  setup();

  sock_create_fake.return_val=0;
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(rd,-1,"Client not connected");
}


void test_http_read_fail_not_client_not_server(void){
  setup();

  sock_create_fake.return_val=0;

  http_client_connect(12,"::");
  http_init_server(12);

  char * buf = (char*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL(fff.call_history[0], (void *)puts);

  TEST_ASSERT_EQUAL_MESSAGE(rd,-1,"Could not read");
}


int main(void){
  UNITY_BEGIN();

  RUN_TEST(test_http_init_server_success);
  RUN_TEST(test_http_init_server_fail_server_init_connection);
  RUN_TEST(test_http_init_server_fail_sock_accept);
  RUN_TEST(test_http_init_server_fail_sock_listen);
  RUN_TEST(test_http_init_server_fail_sock_create);

  RUN_TEST(test_http_client_connect_success);
  RUN_TEST(test_http_client_connect_fail_client_init_connection);
  RUN_TEST(test_http_client_connect_fail_sock_connect);
  RUN_TEST(test_http_client_connect_fail_sock_create);

  RUN_TEST(test_http_disconnect_success_v1);
  RUN_TEST(test_http_disconnect_success_v2);
  RUN_TEST(test_http_disconnect_fail);

  RUN_TEST(test_http_write_success);
  RUN_TEST(test_http_write_two_calls);
  RUN_TEST(test_http_write_fail);

  RUN_TEST(test_http_read_success);
  RUN_TEST(test_http_read_fail_two_calls);
  RUN_TEST(test_http_read_fail_not_connected_client);
  RUN_TEST(test_http_read_fail_not_client_not_server);

  return UNITY_END();
}
