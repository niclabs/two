#include "unit.h"
#include "fff.h"
#include "logging.h"

// Include header definitions for file to test
#include "http2/http2.h"
#include "http2/structs.h"

// Load internal function definitions
// extern int init_variables_h2s(h2states_t *h2s, uint8_t is_server);

// Define list of fake functions
DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, cbuf_len, cbuf_t *);
FAKE_VALUE_FUNC(int, cbuf_pop, cbuf_t *, void *, int);
FAKE_VALUE_FUNC(int8_t, hpack_init_states, hpack_states_t *, uint32_t);
FAKE_VOID_FUNC(send_connection_error, cbuf_t *, uint32_t, h2states_t *);
FAKE_VALUE_FUNC(callback_t, null_callback);

#define FFF_FAKES_LIST(FAKE) \
    FAKE(cbuf_len) \
    FAKE(cbuf_pop) \
    FAKE(hpack_init_states) \
    FAKE(null_callback) \
    FAKE(send_connection_error)


// Reset fakes and memory before each test
void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}



int cbuf_pop_preface_fake(cbuf_t *cbuf, void *dst, int len)
{
    memcpy(dst, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24);
    return 24;
}

void test_example(void)
{
    TEST_FAIL();
}

void test_http2_server_init_connection_good_preface(void)
{
    // Initialize variable memory
    cbuf_t cbuf_in;
    cbuf_t cbuf_out;
    h2states_t state;

    // Set correct return value for http2 functions
    cbuf_len_fake.return_val = 24;
    cbuf_pop_fake.custom_fake = cbuf_pop_preface_fake;

    // Call function
    http2_server_init_connection(&cbuf_in, &cbuf_out, &state);

    TEST_ASSERT_EQUAL_MESSAGE(1, state.is_server, "On correct preface given http2_server_init_connection should initialize state memory as a server");
    TEST_ASSERT_EQUAL_MESSAGE(DEFAULT_MAX_FRAME_SIZE, state.remote_settings[4], "On correct preface given http2_server_init_connection should initialize base settings");
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_example);
    UNIT_TEST(test_http2_server_init_connection_good_preface);

    return UNIT_TESTS_END();
}
