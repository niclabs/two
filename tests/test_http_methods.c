#include "unit.h"

#include "sock.h"

#include "fff.h"
#include "http_methods.h"
#include "http2.h"

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


void setUp(){
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
  strcpy(buf, "hola");
  (void) s;
  (void) u;
  return 4;
}
int sock_destroy_custom_fake(sock_t * s){
  s->state=SOCK_CLOSED;
  return 0;
}


void test_http_init_server_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
  TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

  TEST_ASSERT_EQUAL(0, is);
}


void test_http_init_server_fail_server_init_connection(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
  TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Problems sending server data");
}


void test_http_init_server_fail_sock_accept(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Not client found");
}


void test_http_init_server_fail_sock_listen(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Partial error in server creation");
}


void test_http_init_server_fail_sock_create(void){
  sock_create_fake.return_val=-1;

  int is=http_init_server(12);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, is, "Server could not be created");
}


void test_http_client_connect_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL(0, cc);
}


void test_http_client_connect_fail_client_init_connection(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Problems sending client data");
}


void test_http_client_connect_fail_sock_connect(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client connection");
}


void test_http_client_connect_fail_sock_create(void){
  sock_create_fake.return_val=-1;

  int cc = http_client_connect(12,"::");

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, cc, "Error on client creation");
}


void test_http_disconnect_success_v1(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  http_client_connect(12,"::");

  int d=http_client_disconnect();

  TEST_ASSERT_EQUAL(0, d);
}


void test_http_disconnect_success_v2(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_destroy_fake.custom_fake=sock_destroy_custom_fake;

  int d=http_client_disconnect();


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);

  TEST_ASSERT_EQUAL(0, d);
}


void test_http_disconnect_fail(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_destroy_fake.return_val=-1;

  int d=http_client_disconnect();


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL((void *)sock_destroy, fff.call_history[3]);

  TEST_ASSERT_EQUAL_MESSAGE(-1, d, "Client could not be disconnected");
}


void test_http_write_server_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;

  http_init_server(12);

  sock_write_fake.return_val=4;

  uint8_t * buf = (uint8_t*)malloc(256);
  buf=(uint8_t *)"hola";
  int wr = http_write(buf,256);


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
  TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

  TEST_ASSERT_EQUAL((void *)sock_write, fff.call_history[4]);

  TEST_ASSERT_EQUAL(4, wr);
}


void test_http_write_client_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");

  sock_write_fake.return_val=4;

  uint8_t * buf = (uint8_t*)malloc(256);
  buf=(uint8_t *)"hola";
  int wr = http_write(buf,256);


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL((void *)sock_write, fff.call_history[3]);

  TEST_ASSERT_EQUAL(4, wr);
}


void test_http_write_two_calls(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_write_fake.return_val=0;

  uint8_t * buf = (uint8_t*)malloc(256);
  buf=(uint8_t *)"hola";
  int wr = http_write(buf,256);


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL((void *)sock_write, fff.call_history[3]);

  TEST_ASSERT_EQUAL_MESSAGE(0, wr,"Could not read");
}


void test_http_write_fail_no_client_or_server(void){
  sock_create_fake.return_val=0;
  sock_create_fake.return_val=-1;

  http_client_connect(12,"::");
  http_init_server(12);

  uint8_t * buf = (uint8_t*)malloc(256);
  buf=(uint8_t *)"hola";
  int wr = http_write(buf,256);

  TEST_ASSERT_EQUAL_MESSAGE(-1, wr, "No server or client found");
}


void test_http_read_server_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_listen_fake.custom_fake=sock_listen_custom_fake;
  sock_accept_fake.custom_fake=sock_accept_custom_fake;
  server_init_connection_fake.return_val=0;

  http_init_server(12);

  sock_read_fake.return_val=4;
  sock_read_fake.custom_fake=sock_read_custom_fake;

  uint8_t * buf = (uint8_t*)malloc(256);
  int rd = http_read(buf,256);


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_listen, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)sock_accept, fff.call_history[2]);
  TEST_ASSERT_EQUAL((void *)server_init_connection, fff.call_history[3]);

  TEST_ASSERT_EQUAL((void *)sock_read, fff.call_history[4]);

  TEST_ASSERT_EQUAL(0, memcmp(buf,(uint8_t*)"hola",4));

  TEST_ASSERT_EQUAL(4, rd);
}


void test_http_read_client_success(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;


  http_client_connect(12,"::");

  sock_read_fake.return_val=4;
  sock_read_fake.custom_fake=sock_read_custom_fake;

  uint8_t * buf = (uint8_t*)malloc(256);
  int rd = http_read(buf,256);


  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL((void *)sock_read, fff.call_history[3]);

  TEST_ASSERT_EQUAL(0, memcmp(buf,(uint8_t*)"hola",4));

  TEST_ASSERT_EQUAL(4, rd);
}



void test_http_read_fail_two_calls(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.custom_fake=sock_connect_custom_fake;
  client_init_connection_fake.return_val=0;

  http_client_connect(12,"::");


  sock_read_fake.return_val=0;

  uint8_t * buf = (uint8_t*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL((void *)sock_create, fff.call_history[0]);
  TEST_ASSERT_EQUAL((void *)sock_connect, fff.call_history[1]);
  TEST_ASSERT_EQUAL((void *)client_init_connection, fff.call_history[2]);

  TEST_ASSERT_EQUAL(fff.call_history[3], (void *)sock_read);

  TEST_ASSERT_EQUAL_MESSAGE(0, rd, "Could not read");
}

void test_http_read_fail_not_connected_client(void){
  sock_create_fake.custom_fake=sock_create_custom_fake;
  sock_connect_fake.return_val=-1;

  http_client_connect(12,"::");

  uint8_t * buf = (uint8_t*)malloc(256);
  int rd = http_read(buf,256);

  TEST_ASSERT_EQUAL_MESSAGE(-1, rd, "Client not connected");
}


void test_http_read_fail_no_client_or_server(void){
  sock_create_fake.return_val=0;
  sock_create_fake.return_val=-1;

  http_client_connect(12,"::");
  http_init_server(12);

  uint8_t * buf = (uint8_t*)malloc(256);
  int rd = http_read(buf,256);


  TEST_ASSERT_EQUAL_MESSAGE(-1, rd, "No server or client found");
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

  RUN_TEST(test_http_write_server_success);
  RUN_TEST(test_http_write_client_success);
  RUN_TEST(test_http_write_two_calls);
  RUN_TEST(test_http_write_fail_no_client_or_server);

  RUN_TEST(test_http_read_server_success);
  RUN_TEST(test_http_read_client_success);
  RUN_TEST(test_http_read_fail_two_calls);
  RUN_TEST(test_http_read_fail_not_connected_client);
  RUN_TEST(test_http_read_fail_no_client_or_server);

  return UNITY_END();
}
