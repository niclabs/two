#include "unit.h"
#include "fff.h"

#include "net.h"
#include "sock_non_blocking.h"
#include "cbuf.h"

typedef unsigned int uint;

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_create, sock_t*);
FAKE_VALUE_FUNC(int, sock_destroy, sock_t*);
FAKE_VALUE_FUNC(uint, sock_poll, sock_t*);
FAKE_VALUE_FUNC(int, sock_read, sock_t*, uint8_t*, unsigned int);
FAKE_VALUE_FUNC(int, sock_write, sock_t*, uint8_t*, unsigned int);
FAKE_VALUE_FUNC(int, sock_listen, sock_t*, uint16_t);
FAKE_VALUE_FUNC(int, sock_accept, sock_t*, sock_t*);
FAKE_VOID_FUNC(cbuf_init, cbuf_t*, void*, int);
FAKE_VALUE_FUNC(int, cbuf_push, cbuf_t*, void*, int);
FAKE_VALUE_FUNC(int, cbuf_pop, cbuf_t*, void*, int);
FAKE_VALUE_FUNC(int, cbuf_peek, cbuf_t*, void*, int);
FAKE_VALUE_FUNC(int, cbuf_len, cbuf_t*);
FAKE_VALUE_FUNC(int, cbuf_maxlen, cbuf_t*);

#define FFF_FAKES_LIST(FAKE)            \
    FAKE(sock_accept)                   \
    FAKE(sock_destroy)                  \
    FAKE(sock_poll)                     \
    FAKE(sock_read)                     \
    FAKE(sock_write)                    \
    FAKE(sock_listen)                   \
    FAKE(sock_accept)                   \
    FAKE(cbuf_init)                     \
    FAKE(cbuf_push)                     \
    FAKE(cbuf_pop)                      \
    FAKE(cbuf_peek)                     \
    FAKE(cbuf_len)                      \
    FAKE(cbuf_maxlen)

void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

/*
================ CBUF WRAPPERS ================
*/

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

void cbuf_init_custom_fake(cbuf_t * cbuf, void * buf, int maxlen)
{
    cbuf->ptr = buf;
    cbuf->readptr = buf;
    cbuf->writeptr = buf;
    cbuf->maxlen = maxlen;
    cbuf->len = 0;
}

int cbuf_push_custom_fake(cbuf_t * cbuf, void * src, int len)
{
   int bytes = 0;
    while (len > 0 && cbuf->len < cbuf->maxlen) {
        int copylen = MIN(len, cbuf->maxlen - (cbuf->writeptr - cbuf->ptr));
        if (cbuf->writeptr < cbuf->readptr) {
            copylen = MIN(len, cbuf->readptr - cbuf->writeptr);
        }
        memcpy(cbuf->writeptr, src, copylen);

        // Update write pointer
        cbuf->writeptr = cbuf->ptr + (cbuf->writeptr - cbuf->ptr + copylen) % cbuf->maxlen;

        // Update used count
        cbuf->len += copylen;

        // Update result
        bytes += copylen;

        // Update pointers
        src += copylen;
        len -= copylen;
    }

    return bytes;
}

int cbuf_pop_custom_fake(cbuf_t * cbuf, void * dst, int len)
{
    int bytes = 0;

    while (len > 0 && cbuf->len > 0) {
        int copylen = MIN(len, cbuf->maxlen - (cbuf->readptr - cbuf->ptr));
        if (cbuf->readptr < cbuf->writeptr) {
            copylen = MIN(len, cbuf->writeptr - cbuf->readptr);
        }
       
        // if dst is NULL, calls to read will only increase the read pointer
        if (dst != NULL) { 
            memcpy(dst, cbuf->readptr, copylen);
        }

        // Update read pointer
        cbuf->readptr = cbuf->ptr + (cbuf->readptr - cbuf->ptr + copylen) % cbuf->maxlen;

        // Update used count
        cbuf->len -= copylen;

        // update result
        bytes += copylen;

        // Update pointers
        dst += copylen;
        len -= copylen;
    }

    return bytes;
}

int cbuf_peek_custom_fake(cbuf_t * cbuf, void * dst, int len)
{
    int bytes = 0;
    int cbuflen = cbuf->len;
    void * readptr = cbuf->readptr;

    while (len > 0 && cbuflen > 0) {
        int copylen = MIN(len, cbuf->maxlen - (readptr - cbuf->ptr));
        if (readptr < cbuf->writeptr) {
            copylen = MIN(len, cbuf->writeptr - readptr);
        }
        memcpy(dst, readptr, copylen);

        // Update read pointer
        readptr = cbuf->ptr + (readptr - cbuf->ptr + copylen) % cbuf->maxlen;

        // Update used count
        cbuflen -= copylen;

        // update result
        bytes += copylen;

        // Update pointers
        dst += copylen;
        len -= copylen;
    }

    return bytes;
}

int cbuf_len_custom_fake(cbuf_t * cbuf)
{
    return cbuf->len;
}

int cbuf_maxlen_custom_fake(cbuf_t * cbuf)
{
    return cbuf->maxlen;
}

/*
================ FAKES ================
*/

int fake_stop_flag = 0;

typedef struct test_state_struct {
    int connected;
    int sent_counter;
    int received_counter;
}* test_state_t;

int sock_accept_custom_fake(sock_t* server_socket, sock_t* client_socket)
{
    if (sock_accept_fake.call_count == 1)
    {
        return 1;
    }
    return 0;
}

unsigned int sock_poll_custom_fake(sock_t* socket)
{
    return 13;
}

int sock_read_custon_fake(sock_t* socket, uint8_t * buffer, unsigned int buffer_length)
{
    const char* hello = "Hello World!";
    if (buffer_length == 13)
    {
        strncpy((char*)buffer, hello, 13);
        return 13;
    }
    return 0;
}

char default_callback_fake_data[13];

callback_t default_callback_fake(cbuf_t* buf_in, cbuf_t* buf_out, void* state)
{
    test_state_t st = (test_state_t)state;

    callback_t result;
    result.func = default_callback_fake;

    if (!st->connected)
    {
        st->connected = 1;
    }
    else
    {
        cbuf_pop(buf_in, default_callback_fake_data, 13);
        result.func = NULL;
        fake_stop_flag = 1;
    }

    return result;
}

void test_net_server_loop_connect(void)
{
    unsigned int port = 8888;
    callback_t default_callback;
    default_callback.func = default_callback_fake;
    fake_stop_flag = 0;
    size_t data_buffer_size = 13;
    size_t client_state_size = 4;

    cbuf_init_fake.custom_fake = cbuf_init_custom_fake;
    cbuf_push_fake.custom_fake = cbuf_push_custom_fake;
    cbuf_pop_fake.custom_fake = cbuf_pop_custom_fake;
    cbuf_peek_fake.custom_fake = cbuf_peek_custom_fake;
    cbuf_len_fake.custom_fake = cbuf_len_custom_fake;
    cbuf_maxlen_fake.custom_fake = cbuf_maxlen_custom_fake;

    sock_accept_fake.custom_fake = sock_accept_custom_fake;
    sock_poll_fake.custom_fake = sock_poll_custom_fake;
    sock_read_fake.custom_fake = sock_read_custon_fake;

    NetReturnCode rc = net_server_loop(port, default_callback, &fake_stop_flag, data_buffer_size, client_state_size);

    TEST_ASSERT_EQUAL(Ok, rc);

    TEST_ASSERT_EQUAL(1, sock_create_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_listen_fake.call_count);
    TEST_ASSERT_EQUAL(NET_MAX_CLIENTS, sock_accept_fake.call_count);

    TEST_ASSERT_EQUAL(1, sock_poll_fake.call_count);
    TEST_ASSERT_EQUAL(1, sock_read_fake.call_count);
    
    TEST_ASSERT_EQUAL(1, sock_destroy_fake.call_count);

    TEST_ASSERT_EQUAL_STRING("Hello World!", default_callback_fake_data);
}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_net_server_loop_connect);

    return UNITY_END();
}
