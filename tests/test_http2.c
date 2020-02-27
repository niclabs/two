#include "fff.h"
#include "frames.h"
#include "http2.h"
#include "unit.h"

extern int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
extern int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);
extern int receiving(event_sock_t *sock, int size, uint8_t *buf);
extern void http2_on_client_close(event_sock_t *sock);
extern void on_settings_sent(event_sock_t *sock, int status);
extern int receiving(event_sock_t *client, int size, uint8_t *buf);

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

void test_recv_a_correct_preface(void)
{
    event_sock_t client;
    http2_new_client(&client);

    uint8_t *buf = (uint8_t *)"PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

    // the method consumes the whole buffer
    TEST_ASSERT_EQUAL(24, waiting_for_preface(&client, 24, buf));

    // check that http2 recv settings
    TEST_ASSERT_EQUAL(1, send_settings_frame_fake.call_count);
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.arg1_val); // not an ack

    // the server goes to waiting for settings state
    TEST_ASSERT_EQUAL(1, event_read_fake.call_count);
    TEST_ASSERT_EQUAL(waiting_for_settings, event_read_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_settings_sent(void)
{
    event_sock_t client;
    http2_new_client(&client);

    on_settings_sent(&client, 0);

    // check that ack timer has been set
    TEST_ASSERT_EQUAL(1, event_timer_set_fake.call_count);
    TEST_ASSERT_EQUAL(HTTP2_SETTINGS_WAIT, event_timer_set_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_a_bad_preface(void)
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

void test_recv_settings(void)
{
    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 36] = { // header length 12
                            0,
                            0,
                            6 * 6,
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
                            // setting value = 77
                            0,
                            0,
                            0,
                            77,
                            // settings id = enable push
                            0,
                            0x2,
                            // settings value = 1
                            0,
                            0,
                            0,
                            0,
                            // settings id = max concurrent streams
                            0,
                            0x3,
                            // settings value = 5
                            0,
                            0,
                            0,
                            5,
                            // settings id = initial window size
                            0,
                            0x4,
                            // settings value = 122
                            0,
                            0,
                            0,
                            127,
                            // settings id = max frame size
                            0,
                            0x5,
                            // settings value = 16385
                            0,
                            0,
                            0x40,
                            0x01,
                            // settings id = max header list size
                            0,
                            0x6,
                            // settings value = 256
                            0,
                            0,
                            0x01,
                            0

    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(45, waiting_for_settings(&client, 45, buf));

    // check that the server updated the client context settings
    TEST_ASSERT_EQUAL(77, ctx->settings.header_table_size);
    TEST_ASSERT_EQUAL(0, ctx->settings.enable_push);
    TEST_ASSERT_EQUAL(5, ctx->settings.max_concurrent_streams);
    TEST_ASSERT_EQUAL(127, ctx->settings.initial_window_size);
    TEST_ASSERT_EQUAL(16385, ctx->settings.max_frame_size);
    TEST_ASSERT_EQUAL(256, ctx->settings.max_header_list_size);

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

void test_recv_ping_while_waiting_for_settings(void)
{
    event_sock_t client;
    http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 8] = { // header length 8 for ping
                           0,
                           0,
                           8,
                           // frame type
                           FRAME_PING_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // opaque data
                           0,
                           0,
                           0,
                           0,
                           0,
                           0,
                           0,
                           0
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(17, waiting_for_settings(&client, 17, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // the server must send protocol error and stop receiving
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, send_goaway_frame_fake.call_count);
    TEST_ASSERT_EQUAL(&client, send_goaway_frame_fake.arg0_val);
    TEST_ASSERT_EQUAL(HTTP2_PROTOCOL_ERROR, send_goaway_frame_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_settings_max_frame_size_too_small(void)
{

    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 6] = { // header length 12
                           0,
                           0,
                           6 * 1,
                           // frame type settings
                           FRAME_SETTINGS_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // settings id = max frame size
                           0,
                           0x5,
                           // settings value = 256
                           0,
                           0,
                           0x01,
                           0
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(15, receiving(&client, 15, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // check that the server maintains max fram size
    TEST_ASSERT_EQUAL(16384, ctx->settings.max_frame_size);

    // the server must send protocol error and stop receiving
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, send_goaway_frame_fake.call_count);
    TEST_ASSERT_EQUAL(&client, send_goaway_frame_fake.arg0_val);
    TEST_ASSERT_EQUAL(HTTP2_PROTOCOL_ERROR, send_goaway_frame_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_settings_max_frame_size_too_large(void)
{

    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 6] = { // header length 12
                           0,
                           0,
                           6 * 1,
                           // frame type settings
                           FRAME_SETTINGS_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // settings id = max frame size
                           0,
                           0x5,
                           // settings value = 2^24
                           0x1,
                           0,
                           0,
                           0
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(15, receiving(&client, 15, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // check that the server maintains max fram size
    TEST_ASSERT_EQUAL(16384, ctx->settings.max_frame_size);

    // the server must send protocol error and stop receiving
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, send_goaway_frame_fake.call_count);
    TEST_ASSERT_EQUAL(&client, send_goaway_frame_fake.arg0_val);
    TEST_ASSERT_EQUAL(HTTP2_PROTOCOL_ERROR, send_goaway_frame_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_settings_enable_push_wrong_value(void)
{

    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 6] = { // header length 12
                           0,
                           0,
                           6 * 1,
                           // frame type settings
                           FRAME_SETTINGS_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // settings id = max frame size
                           0,
                           0x2,
                           // settings value = 2
                           0,
                           0,
                           0,
                           2
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(15, receiving(&client, 15, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // check that the server maintains max fram size
    TEST_ASSERT_EQUAL(1, ctx->settings.enable_push);

    // the server must send protocol error and stop receiving
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, send_goaway_frame_fake.call_count);
    TEST_ASSERT_EQUAL(&client, send_goaway_frame_fake.arg0_val);
    TEST_ASSERT_EQUAL(HTTP2_PROTOCOL_ERROR, send_goaway_frame_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_settings_initial_window_size_too_large(void)
{
    event_sock_t client;
    http2_context_t *ctx = http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 6] = { // header length 12
                           0,
                           0,
                           6 * 1,
                           // frame type settings
                           FRAME_SETTINGS_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // settings id = max frame size
                           0,
                           0x4,
                           // settings value = 2^31
                           0x80,
                           0,
                           0,
                           0
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(15, receiving(&client, 15, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // check that the server maintains max fram size
    TEST_ASSERT_EQUAL(65535, ctx->settings.initial_window_size);

    // the server must send protocol error and stop receiving
    TEST_ASSERT_EQUAL(1, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(&client, event_read_stop_fake.arg0_val);
    TEST_ASSERT_EQUAL(1, send_goaway_frame_fake.call_count);
    TEST_ASSERT_EQUAL(&client, send_goaway_frame_fake.arg0_val);
    TEST_ASSERT_EQUAL(HTTP2_FLOW_CONTROL_ERROR,
                      send_goaway_frame_fake.arg1_val);

    // close client
    http2_on_client_close(&client);
}

void test_recv_settings_with_unknown_identifier(void)
{
    event_sock_t client;
    http2_new_client(&client);

    // use custom header parsing function
    frame_parse_header_fake.custom_fake = parse_header;

    // prepare settings frame
    uint8_t buf[9 + 6] = { // header length 12
                           0,
                           0,
                           6 * 1,
                           // frame type settings
                           FRAME_SETTINGS_TYPE,
                           // no flags
                           0,
                           // stream id 0 (with reserved bit)
                           0x80,
                           0,
                           0,
                           0,
                           // settings id = unknown
                           0,
                           0x7,
                           // settings value = 65536
                           0,
                           1,
                           0,
                           0
    };

    // the method consumed all bytes
    TEST_ASSERT_EQUAL(15, receiving(&client, 15, buf));

    // the server never recv settings ack
    TEST_ASSERT_EQUAL(0, send_settings_frame_fake.call_count);

    // the server does not send an error
    TEST_ASSERT_EQUAL(0, event_read_stop_fake.call_count);
    TEST_ASSERT_EQUAL(0, send_goaway_frame_fake.call_count);

    // close client
    http2_on_client_close(&client);
}

int main(void)

{
    UNITY_BEGIN();
    // connection preface tests
    UNIT_TEST(test_recv_a_correct_preface);
    UNIT_TEST(test_recv_a_bad_preface);

    // settings tests
    UNIT_TEST(test_settings_sent);
    UNIT_TEST(test_recv_settings);
    UNIT_TEST(test_recv_ping_while_waiting_for_settings);
    UNIT_TEST(test_recv_settings_max_frame_size_too_small);
    UNIT_TEST(test_recv_settings_max_frame_size_too_large);
    UNIT_TEST(test_recv_settings_enable_push_wrong_value);
    UNIT_TEST(test_recv_settings_initial_window_size_too_large);
    UNIT_TEST(test_recv_settings_with_unknown_identifier);
    return UNITY_END();
}
