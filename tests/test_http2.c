#include "fff.h"
#include "frames.h"
#include "http2.h"
#include "unit.h"

extern int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
extern int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);
extern int receiving(event_sock_t *sock, int size, uint8_t *buf);
extern void http2_on_client_close(event_sock_t *sock);

DEFINE_FFF_GLOBALS;

// event.h fakes
FAKE_VOID_FUNC(event_read_start,
               event_sock_t *,
               uint8_t *,
               unsigned int,
               event_read_cb);
FAKE_VOID_FUNC(event_write_enable, event_sock_t *, uint8_t *, unsigned int);
FAKE_VOID_FUNC(event_timer_reset, event_t *);
FAKE_VOID_FUNC(event_timer_stop, event_t *);
FAKE_VALUE_FUNC(int, event_close, event_sock_t *, event_close_cb);
FAKE_VALUE_FUNC(int, event_read, event_sock_t *, event_read_cb);
FAKE_VALUE_FUNC(int, event_read_stop, event_sock_t *);
FAKE_VALUE_FUNC(event_t *,
                event_timer_set,
                event_sock_t *,
                unsigned int,
                event_timer_cb);

// hpack fakes
FAKE_VOID_FUNC(hpack_init, hpack_dynamic_table_t *, uint32_t);
FAKE_VOID_FUNC(hpack_dynamic_change_max_size,
               hpack_dynamic_table_t *,
               uint32_t);
FAKE_VALUE_FUNC(int,
                hpack_decode,
                hpack_dynamic_table_t *,
                uint8_t *,
                int,
                header_list_t *);

// frame fakes
FAKE_VALUE_FUNC(int, frame_header_to_bytes, frame_header_t *, uint8_t *);
FAKE_VOID_FUNC(frame_parse_header, frame_header_t *, uint8_t *, unsigned int);
FAKE_VALUE_FUNC(int,
                send_goaway_frame,
                event_sock_t *,
                uint32_t,
                uint32_t,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                send_settings_frame,
                event_sock_t *,
                int,
                uint32_t *,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                send_ping_frame,
                event_sock_t *,
                uint8_t *,
                int,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                send_rst_stream_frame,
                event_sock_t *,
                uint32_t,
                uint32_t,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                send_headers_frame,
                event_sock_t *,
                header_list_t *,
                hpack_dynamic_table_t *,
                uint32_t,
                uint8_t,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                send_data_frame,
                event_sock_t *,
                uint8_t *,
                uint32_t,
                uint32_t,
                uint8_t,
                event_write_cb);

// header list fakes
FAKE_VALUE_FUNC(char *, header_list_get, header_list_t *, const char *);
FAKE_VALUE_FUNC(int,
                header_list_set,
                header_list_t *,
                const char *,
                const char *);
FAKE_VOID_FUNC(header_list_reset, header_list_t *);
FAKE_VALUE_FUNC(unsigned int, header_list_count, header_list_t *);
FAKE_VALUE_FUNC(http_header_t *,
                header_list_all,
                header_list_t *,
                http_header_t *);

// buffer fakes
FAKE_VALUE_FUNC(uint32_t, buffer_get_u31, uint8_t *);

// http fakes
FAKE_VOID_FUNC(http_handle_request,
               http_request_t *,
               http_response_t *,
               unsigned int);

#define FFF_FAKES_LIST(FAKE)                                                   \
    FAKE(event_read_start)                                                     \
    FAKE(event_write_enable)                                                   \
    FAKE(event_close)                                                          \
    FAKE(event_read_stop)                                                      \
    FAKE(event_read)                                                           \
    FAKE(event_timer_reset)                                                    \
    FAKE(event_timer_stop)                                                     \
    FAKE(event_timer_set)                                                      \
    FAKE(hpack_init)                                                           \
    FAKE(hpack_dynamic_change_max_size)                                        \
    FAKE(hpack_decode)                                                         \
    FAKE(frame_header_to_bytes)                                                \
    FAKE(frame_parse_header)                                                   \
    FAKE(send_goaway_frame)                                                    \
    FAKE(send_settings_frame)                                                  \
    FAKE(send_ping_frame)                                                      \
    FAKE(send_rst_stream_frame)                                                \
    FAKE(send_headers_frame)                                                   \
    FAKE(send_data_frame)                                                      \
    FAKE(buffer_get_u31)                                                       \
    FAKE(header_list_reset)                                                    \
    FAKE(header_list_count)                                                    \
    FAKE(header_list_all)                                                      \
    FAKE(header_list_set)                                                      \
    FAKE(header_list_get)                                                      \
    FAKE(http_handle_request)

void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

uint32_t read_u31(uint8_t *bytes)
{
    return ((bytes[0] & 0x7F) << 24) + (bytes[1] << 16) + (bytes[2] << 8) +
           bytes[3];
}

uint32_t read_u24(uint8_t *bytes)
{
    return (bytes[0] << 16) + (bytes[1] << 8) + bytes[2];
}

uint8_t read_u8(uint8_t *bytes)
{
    return bytes[0];
}

void parse_header(frame_header_t *header, uint8_t *data, unsigned int size)
{
    (void)size;

    // cleanup memory first
    memset(header, 0, sizeof(frame_header_t));

    header->length    = read_u24(data);
    header->type      = read_u8(data + 3);
    header->flags     = read_u8(data + 4);
    header->stream_id = read_u31(data + 5);
}

void test_handle_good_preface(void)
{
    event_sock_t client;
    http2_new_client(&client);

    uint8_t *buf = (uint8_t *)"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

    // the method consumes the whole buffer
    TEST_ASSERT_EQUAL(24, waiting_for_preface(&client, 24, buf));

    // check that http2 sends settings
    TEST_ASSERT_EQUAL(1, send_settings_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.arg1_val); // not an ack

    // the server goes to waiting for settings state
    TEST_ASSERT_EQUAL(1, event_read_fake.call_count);
    TEST_ASSERT_EQUAL(waiting_for_settings, event_read_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_handle_bad_preface(void)
{
    event_sock_t client;
    http2_new_client(&client);

    uint8_t *buf = (uint8_t *)"this is a bad preface!!!";

    // the method consumes the whole buffer
    TEST_ASSERT_EQUAL(24, waiting_for_preface(&client, 24, buf));

    // on bad preface the server must close the client
    TEST_ASSERT_EQUAL(1, event_close_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_close_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);

    // the server must not send settings
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.arg1_val); // not an ack

    // check that the server does not move to next state
    TEST_ASSERT_EQUAL(0, event_read_fake.call_count);

    // close client
    http2_on_client_close(&client);
}

void test_receive_settings(void)
{
    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 12] = { // header length 12
                            0,
                            0,
                            6 + 6,
                            // frame type settings
                            FRAME_SETTINGS_TYPE,
                            // no flags
                            0,
                            // stream id 0 (with reserved bit)
                            0x80,
                            0,
                            0,
                            0,
                            // settings id = header table size
                            0,
                            0x1,
                            // setting value = 128
                            0,
                            0,
                            0,
                            77,
                            // settings id = initial window size
                            0,
                            0x4,
                            // settings value = 122
                            0,
                            0,
                            0,
                            127

    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(21, waiting_for_settings(&client, 21, buf));

    // the server updates the client context settings
    ctx->settings.header_table_size   = 77;
    ctx->settings.initial_window_size = 127;

    // the server updates the dynamic table size
    TEST_ASSERT_EQUAL(1, hpack_dynamic_change_max_size_fake.call_count);
    TEST_ASSERT_EQUAL(77, hpack_dynamic_change_max_size_fake.arg1_val);

    // the server replies with ack
    TEST_ASSERT_EQUAL(1, send_settings_frame_fake.call_count);
    TEST_ASSERT_EQUAL(1, send_settings_frame_fake.arg1_val);

    // the server moved to receiving state
    TEST_ASSERT_EQUAL(1, event_read_fake.call_count);
    TEST_ASSERT_EQUAL(receiving, event_read_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

int main(void)

{
    UNITY_BEGIN();
    UNIT_TEST(test_handle_good_preface);
    UNIT_TEST(test_handle_bad_preface);
    UNIT_TEST(test_receive_settings);
    return UNITY_END();
}
