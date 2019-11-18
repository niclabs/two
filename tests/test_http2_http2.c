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
FAKE_VOID_FUNC(headers_init, header_list_t *);
FAKE_VALUE_FUNC(int, send_local_settings, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, frame_header_from_bytes, uint8_t *, int, frame_header_t *);
FAKE_VALUE_FUNC(int, read_setting_from, h2states_t *, uint8_t, uint8_t);
FAKE_VALUE_FUNC(int, check_incoming_data_condition, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, check_incoming_headers_condition, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, check_incoming_settings_condition, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, check_incoming_goaway_condition, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, check_incoming_continuation_condition, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, handle_data_payload, frame_header_t *, data_payload_t *, cbuf_t *, h2states_t*);
FAKE_VALUE_FUNC(int, handle_headers_payload, frame_header_t *, headers_payload_t *, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, handle_settings_payload, settings_payload_t *, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, handle_goaway_payload, goaway_payload_t *, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, handle_continuation_payload, frame_header_t *, continuation_payload_t *, cbuf_t *, h2states_t *);
FAKE_VALUE_FUNC(int, handle_window_update_payload, window_update_payload_t *, cbuf_t *, h2states_t *);


#define FFF_FAKES_LIST(FAKE) \
    FAKE(cbuf_len) \
    FAKE(cbuf_pop) \
    FAKE(hpack_init_states) \
    FAKE(send_connection_error) \
    FAKE(null_callback) \
    FAKE(headers_init) \
    FAKE(send_local_settings) \
    FAKE(frame_header_from_bytes) \
    FAKE(read_setting_from) \
    FAKE(check_incoming_data_condition) \
    FAKE(check_incoming_headers_condition) \
    FAKE(check_incoming_settings_condition) \
    FAKE(check_incoming_goaway_condition) \
    FAKE(check_incoming_continuation_condition) \
    FAKE(handle_data_payload) \
    FAKE(handle_headers_payload) \
    FAKE(handle_settings_payload) \
    FAKE(handle_goaway_payload) \
    FAKE(handle_continuation_payload) \
    FAKE(handle_window_update_payload)


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

    UNIT_TEST(test_http2_server_init_connection_good_preface);

    return UNIT_TESTS_END();
}
