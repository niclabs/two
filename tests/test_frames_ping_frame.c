//
// Created by gabriel on 18-11-19.
//
#include "unit.h"
#include "logging.h"
#include "frames/common.h" //Common structs for frames
#include "ping_frame.h"
#include "fff.h"


// Include header definitions for file to test
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t *);



/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(buffer_copy)                 \
    FAKE(bytes_to_uint32_24)          \
    FAKE(bytes_to_uint32_31)

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


/* Mocks */
int buffer_copy_fake_custom(uint8_t *dest, uint8_t *orig, int size)
{
    for (int i = 0; i < size; i++) {
        dest[i] = orig[i];
    }
    return size;
}

uint8_t set_flag_fake_custom(uint8_t flags, uint8_t flag_to_set)
{
    uint8_t new_flag = flags | flag_to_set;

    return new_flag;
}


void test_create_ping_frame(void)
{
    uint8_t payload[8] = { 'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r' };
    int ping_size = 8;
    frame_header_t frame_header;
    ping_payload_t ping_payload;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_ping_frame(&frame_header, &ping_payload, payload);

    TEST_ASSERT_EQUAL(0, frame_header.stream_id);
    TEST_ASSERT_EQUAL(PING_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0, frame_header.flags);
    TEST_ASSERT_EQUAL(ping_size, frame_header.length);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(payload[i], ping_payload.opaque_data[i]);
    }
}

void test_create_ping_ack_frame(void)
{
    uint8_t payload[8] = { 'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r' };
    int ping_size = 8;
    frame_header_t frame_header;
    ping_payload_t ping_payload;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_ping_ack_frame(&frame_header, &ping_payload, payload);

    TEST_ASSERT_EQUAL(0, frame_header.stream_id);
    TEST_ASSERT_EQUAL(PING_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(PING_ACK_FLAG, frame_header.flags);
    TEST_ASSERT_EQUAL(ping_size, frame_header.length);
    for (int i = 0; i < 8; i++) {
        TEST_ASSERT_EQUAL(payload[i], ping_payload.opaque_data[i]);
    }
}

void test_ping_payload_to_bytes(void)
{
    uint8_t payload[8] = { 'h', 'e', 'l', 'l', 'o', 'w', 'o', 'r' };
    uint8_t byte_array[8];

    memset(byte_array, 0, 8);
    int ping_size = 8;
    frame_header_t frame_header;
    ping_payload_t ping_payload;


    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_ping_frame(&frame_header, &ping_payload, payload);

    int rc = ping_payload_to_bytes(&frame_header, &ping_payload, byte_array);
    TEST_ASSERT_EQUAL(ping_size, rc);
    for (int i = 0; i < ping_size; i++) {
        TEST_ASSERT_EQUAL(payload[i], byte_array[i]);
    }
}


void test_read_ping_payload(void)
{
/*expected*/

    frame_header_t expected_frame_header;

    expected_frame_header.type = PING_TYPE;
    expected_frame_header.flags = PING_ACK_FLAG;
    expected_frame_header.stream_id = 0;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 8;
    ping_payload_t expected_ping_payload = { .opaque_data = { 'o', 'p', 'a', 'q', 'u', 'e', 'd', 'a' } };
    //uint8_t expected_padding[32] = {};
    //expected_headers_payload.padding = expected_padding;

/*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_24_fake.return_val = expected_frame_header.length;
    bytes_to_uint32_31_fake.return_val = expected_frame_header.stream_id;


    //fill the buffer
    uint8_t read_buffer[8] = { 'o', 'p', 'a', 'q', 'u', 'e', 'd', 'a' };/*payload*/
/*result*/
    ping_payload_t ping_payload;

//uint8_t padding[32]; //not implemented yet!
//int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)

    int rc = read_ping_payload(&expected_frame_header, (void *)&ping_payload, read_buffer);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 8 bytes
    for (int i = 0; i < expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_ping_payload.opaque_data[i], ping_payload.opaque_data[i]);
    }
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_read_ping_payload);
    UNIT_TEST(test_create_ping_frame);
    UNIT_TEST(test_create_ping_ack_frame);
    UNIT_TEST(test_ping_payload_to_bytes);

    return UNIT_TESTS_END();
}
