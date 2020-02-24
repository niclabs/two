#include "buffer.h"
#include "fff.h"

#include "frames.h"
#include "unit.h"

extern int frame_header_to_bytes(frame_header_t *frame_header,
                                 uint8_t *byte_array);

DEFINE_FFF_GLOBALS;
FAKE_VOID_FUNC(buffer_put_u32, uint8_t *, uint32_t);
FAKE_VOID_FUNC(buffer_put_u31, uint8_t *, uint32_t);
FAKE_VOID_FUNC(buffer_put_u24, uint8_t *, uint32_t);
FAKE_VOID_FUNC(buffer_put_u16, uint8_t *, uint16_t);
FAKE_VOID_FUNC(buffer_put_u8, uint8_t *, uint8_t);
FAKE_VALUE_FUNC(int,
                event_write,
                event_sock_t *,
                unsigned int,
                uint8_t *,
                event_write_cb);
FAKE_VALUE_FUNC(int,
                hpack_encode,
                hpack_dynamic_table_t *,
                header_list_t *,
                uint8_t *,
                uint32_t);

#define FFF_FAKES_LIST(FAKE)                                                   \
    FAKE(buffer_put_u32)                                                       \
    FAKE(buffer_put_u31)                                                       \
    FAKE(buffer_put_u24)                                                       \
    FAKE(buffer_put_u16)                                                       \
    FAKE(buffer_put_u8)                                                        \
    FAKE(event_write)                                                          \
    FAKE(hpack_encode)

void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_frame_header_to_bytes(void)
{
    frame_header_t frame_header = { .length    = 8,
                                    .type      = FRAME_PING_TYPE,
                                    .flags     = 0x1,
                                    .reserved  = 1,
                                    .stream_id = 3 };

    uint8_t bytes[9];
    TEST_ASSERT_EQUAL(9, frame_header_to_bytes(&frame_header, bytes));

    // check that first 24 bits are the length
    TEST_ASSERT_EQUAL(frame_header.length, buffer_put_u24_fake.arg1_val);

    // next 8 bits for type
    TEST_ASSERT_EQUAL(frame_header.type, buffer_put_u8_fake.arg1_history[0]);

    // next 8 bits for flags
    TEST_ASSERT_EQUAL(frame_header.flags, buffer_put_u8_fake.arg1_history[1]);

    // next 31 bits for stream_id
    TEST_ASSERT_EQUAL(frame_header.stream_id, buffer_put_u31_fake.arg1_val);

    // reserved bit set to 1
    TEST_ASSERT(bytes[5] & 0x80);
}

void test_send_ping_frame(void)
{
    uint8_t opaque_data[12] = { 'h', 'e', 'l', 'l', 'o', ',',
                                ' ', 'w', 'o', 'r', 'l', 'd' };
    send_ping_frame(NULL, opaque_data, 1, NULL);

    // ping frame size is always 8
    TEST_ASSERT_EQUAL(8, buffer_put_u24_fake.arg1_val);

    // frame type
    TEST_ASSERT_EQUAL(FRAME_PING_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // sending an ack
    TEST_ASSERT_EQUAL(FRAME_FLAGS_ACK, buffer_put_u8_fake.arg1_history[1]);

    // ping frames are connection related
    TEST_ASSERT_EQUAL(0, buffer_put_u31_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 8, event_write_fake.arg1_val);
}

void test_send_goaway_frame(void)
{
    // send a COMPRESSION ERROR
    send_goaway_frame(NULL, 0x9, 11, NULL);

    // goaway frame size is always 8
    TEST_ASSERT_EQUAL(8, buffer_put_u24_fake.arg1_val);

    // goaway frames
    TEST_ASSERT_EQUAL(FRAME_GOAWAY_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // no flags
    TEST_ASSERT_EQUAL(FRAME_FLAGS_NONE, buffer_put_u8_fake.arg1_history[1]);

    // stream id
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // error type
    TEST_ASSERT_EQUAL(0x9, buffer_put_u32_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 8, event_write_fake.arg1_val);
}

void test_send_settings_frame(void)
{
    uint32_t settings_values[6] = { 1, 2, 3, 4, 5, 6 };
    send_settings_frame(NULL, 0, settings_values, NULL);

    // we always send every setting
    TEST_ASSERT_EQUAL(6 * 6, buffer_put_u24_fake.arg1_val);

    // settings type
    TEST_ASSERT_EQUAL(FRAME_SETTINGS_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // no flags (not an ack)
    TEST_ASSERT_EQUAL(FRAME_FLAGS_NONE, buffer_put_u8_fake.arg1_history[1]);

    // stream id should always be 0 for settings
    TEST_ASSERT_EQUAL(0, buffer_put_u31_fake.arg1_val);

    // settings are set in 1,2,3,4,5,6 order
    TEST_ASSERT_EQUAL(0x1, buffer_put_u16_fake.arg1_history[0]);
    TEST_ASSERT_EQUAL(1, buffer_put_u32_fake.arg1_history[0]);
    TEST_ASSERT_EQUAL(0x2, buffer_put_u16_fake.arg1_history[1]);
    TEST_ASSERT_EQUAL(2, buffer_put_u32_fake.arg1_history[1]);
    TEST_ASSERT_EQUAL(0x3, buffer_put_u16_fake.arg1_history[2]);
    TEST_ASSERT_EQUAL(3, buffer_put_u32_fake.arg1_history[2]);
    TEST_ASSERT_EQUAL(0x4, buffer_put_u16_fake.arg1_history[3]);
    TEST_ASSERT_EQUAL(4, buffer_put_u32_fake.arg1_history[3]);
    TEST_ASSERT_EQUAL(0x5, buffer_put_u16_fake.arg1_history[4]);
    TEST_ASSERT_EQUAL(5, buffer_put_u32_fake.arg1_history[4]);
    TEST_ASSERT_EQUAL(0x6, buffer_put_u16_fake.arg1_history[5]);
    TEST_ASSERT_EQUAL(6, buffer_put_u32_fake.arg1_history[5]);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 36, event_write_fake.arg1_val);
}

void test_send_settings_ack(void)
{
    uint32_t settings_values[6] = { 1, 2, 3, 4, 5, 6 };
    send_settings_frame(NULL, 1, settings_values, NULL);

    // length must be 0 for ack
    TEST_ASSERT_EQUAL(0, buffer_put_u24_fake.arg1_val);

    // settings type
    TEST_ASSERT_EQUAL(FRAME_SETTINGS_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // no flags (not an ack)
    TEST_ASSERT_EQUAL(FRAME_FLAGS_ACK, buffer_put_u8_fake.arg1_history[1]);

    // stream id should always be 0 for settings
    TEST_ASSERT_EQUAL(0, buffer_put_u31_fake.arg1_val);

    // no settings are set
    TEST_ASSERT_EQUAL(0, buffer_put_u16_fake.call_count);
    TEST_ASSERT_EQUAL(0, buffer_put_u32_fake.call_count);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9, event_write_fake.arg1_val);
}

void test_send_headers_frame(void)
{
    // set encoded size
    hpack_encode_fake.return_val = 17;

    // send headers frame for stream 11 and end stream
    send_headers_frame(NULL, NULL, NULL, 11, 0, NULL);

    // length must be equal to the encoded size
    TEST_ASSERT_EQUAL(17, buffer_put_u24_fake.arg1_val);

    // headers type
    TEST_ASSERT_EQUAL(FRAME_HEADERS_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // always send end_headers flag
    TEST_ASSERT_EQUAL(FRAME_FLAGS_END_HEADERS,
                      buffer_put_u8_fake.arg1_history[1]);

    // stream id should equal the one given
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 17, event_write_fake.arg1_val);
}

void test_send_headers_end_stream(void)
{
    // set encoded size
    hpack_encode_fake.return_val = 17;

    // send headers frame for stream 11 and end stream
    send_headers_frame(NULL, NULL, NULL, 11, 1, NULL);

    // length must be equal to the encoded size
    TEST_ASSERT_EQUAL(17, buffer_put_u24_fake.arg1_val);

    // headers type
    TEST_ASSERT_EQUAL(FRAME_HEADERS_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // always send end_headers flag
    TEST_ASSERT_EQUAL(FRAME_FLAGS_END_HEADERS | FRAME_FLAGS_END_STREAM,
                      buffer_put_u8_fake.arg1_history[1]);

    // stream id should equal the one given
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 17, event_write_fake.arg1_val);
}

void test_send_headers_hpack_error(void)
{
    // return hpack error
    hpack_encode_fake.return_val = -3;

    // send headers frame for stream 11 and end stream
    int res = send_headers_frame(NULL, NULL, NULL, 11, 0, NULL);

    // send headers should return the hpack error
    TEST_ASSERT_EQUAL(-3, res);

    // check that write is never called
    TEST_ASSERT_EQUAL(0, event_write_fake.call_count);
}

void test_send_window_update_frame(void)
{
    send_window_update_frame(NULL, 137, 11, NULL);

    // window_update frame size is always 4
    TEST_ASSERT_EQUAL(4, buffer_put_u24_fake.arg1_val);

    // goaway frames
    TEST_ASSERT_EQUAL(FRAME_WINDOW_UPDATE_TYPE,
                      buffer_put_u8_fake.arg1_history[0]);

    // no flags
    TEST_ASSERT_EQUAL(FRAME_FLAGS_NONE, buffer_put_u8_fake.arg1_history[1]);

    // stream id
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_history[0]);

    // increment
    TEST_ASSERT_EQUAL(137, buffer_put_u31_fake.arg1_history[1]);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 4, event_write_fake.arg1_val);
}

void test_send_rst_stream_frame(void)
{
    send_rst_stream_frame(NULL, 0x9, 11, NULL);

    // rst_stream frame size is always 4
    TEST_ASSERT_EQUAL(4, buffer_put_u24_fake.arg1_val);

    // rst_stream frame
    TEST_ASSERT_EQUAL(FRAME_RST_STREAM_TYPE,
                      buffer_put_u8_fake.arg1_history[0]);

    // no flags
    TEST_ASSERT_EQUAL(0, buffer_put_u8_fake.arg1_history[1]);

    // stream id
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // increment
    TEST_ASSERT_EQUAL(0x9, buffer_put_u32_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 4, event_write_fake.arg1_val);
}

void test_send_data_frame(void)
{
    uint8_t data[127];
    memset(data, 0, 127);

    // send 127 bytes for stream 11 and end stream
    send_data_frame(NULL, data, 127, 11, 0, NULL);

    // frame size must match
    TEST_ASSERT_EQUAL(127, buffer_put_u24_fake.arg1_val);

    // data frame
    TEST_ASSERT_EQUAL(FRAME_DATA_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // no flags
    TEST_ASSERT_EQUAL(FRAME_FLAGS_NONE, buffer_put_u8_fake.arg1_history[1]);

    // stream id
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 127, event_write_fake.arg1_val);
}

void test_send_data_end_stream(void)
{
    uint8_t data[127];
    memset(data, 0, 127);

    // send 127 bytes for stream 11 and end stream
    send_data_frame(NULL, data, 127, 11, 1, NULL);

    // frame size must match
    TEST_ASSERT_EQUAL(127, buffer_put_u24_fake.arg1_val);

    // data frame
    TEST_ASSERT_EQUAL(FRAME_DATA_TYPE, buffer_put_u8_fake.arg1_history[0]);

    // end stream
    TEST_ASSERT_EQUAL(FRAME_FLAGS_END_STREAM,
                      buffer_put_u8_fake.arg1_history[1]);

    // stream id
    TEST_ASSERT_EQUAL(11, buffer_put_u31_fake.arg1_val);

    // total write size is header + frame size
    TEST_ASSERT_EQUAL(9 + 127, event_write_fake.arg1_val);
}

int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_frame_header_to_bytes);
    UNIT_TEST(test_send_ping_frame);
    UNIT_TEST(test_send_goaway_frame);
    UNIT_TEST(test_send_settings_frame);
    UNIT_TEST(test_send_settings_ack);
    UNIT_TEST(test_send_headers_frame);
    UNIT_TEST(test_send_headers_end_stream);
    UNIT_TEST(test_send_headers_hpack_error);
    UNIT_TEST(test_send_window_update_frame);
    UNIT_TEST(test_send_rst_stream_frame);
    UNIT_TEST(test_send_data_frame);
    UNIT_TEST(test_send_data_end_stream);
    return UNITY_END();
}
