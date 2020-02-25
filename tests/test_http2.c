#include "fff.h"
#include "frames.h"
#include "http2.h"
#include "unit.h"

extern int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
extern int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);

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

    // http2 goes to waitign for settings state
    TEST_ASSERT_EQUAL(1, event_read_fake.call_count);
    TEST_ASSERT_EQUAL(waiting_for_settings, event_read_fake.arg1_val);
}

int main(void)

{
    UNITY_BEGIN();
    UNIT_TEST(test_handle_good_preface);
    return UNITY_END();
}
