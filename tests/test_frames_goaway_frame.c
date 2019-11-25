#include "unit.h"
#include "logging.h"
#include "frames/structs.h" //Common structs for frames
#include "goaway_frame.h"
#include "fff.h"


// Include header definitions for file to test
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(buffer_copy)                 \
    FAKE(uint32_to_byte_array)        \
    FAKE(uint32_31_to_byte_array)     \
    FAKE(bytes_to_uint32_31)          \
    FAKE(bytes_to_uint32_24)          \
    FAKE(bytes_to_uint32)             \

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

int uint32_to_byte_array_custom_fake_num(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = (uint8_t)num;
    return 0;
}

/*Here begin the tests*/
void test_create_goaway_frame(void)
{
    frame_header_t frame_header;
    goaway_payload_t goaway_payload;
    uint8_t additional_debug_data_buffer[8];


    uint32_t last_stream_id = 30;
    uint32_t error_code = 1;
    uint8_t additional_debug_data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    uint8_t additional_debug_data_size = 8;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_goaway_frame(&frame_header,
                        &goaway_payload,
                        additional_debug_data_buffer,
                        last_stream_id,
                        error_code,
                        additional_debug_data,
                        additional_debug_data_size);

    TEST_ASSERT_EQUAL(8 + 8, frame_header.length);
    TEST_ASSERT_EQUAL(GOAWAY_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0x0, frame_header.flags);
    TEST_ASSERT_EQUAL(0, frame_header.stream_id);

    TEST_ASSERT_EQUAL(last_stream_id, goaway_payload.last_stream_id);
    TEST_ASSERT_EQUAL(error_code, goaway_payload.error_code);

    for (int i = 0; i < frame_header.length - 8; i++) {
        TEST_ASSERT_EQUAL(additional_debug_data[i], goaway_payload.additional_debug_data[i]);
    }
}

void test_goaway_payload_to_bytes(void)
{
    frame_header_t frame_header;

    frame_header.length = 16;
    frame_header.type = GOAWAY_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 0;

    goaway_payload_t goaway_payload;
    goaway_payload.last_stream_id = 30;
    goaway_payload.error_code = 1;
    uint8_t additional_debug_data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    goaway_payload.additional_debug_data = additional_debug_data;


    uint8_t expected_bytes[] = { 0, 0, 0, 30,
                                 0, 0, 0, 1,
                                 1, 2, 3, 4, 5, 6, 7, 8 };

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_31_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;

    uint8_t byte_array[16];

    int rc = goaway_payload_to_bytes(&frame_header, &goaway_payload, byte_array);
    TEST_ASSERT_EQUAL(16, rc);
    TEST_ASSERT_EQUAL(16, frame_header.length);

    for (int i = 0; i < frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], byte_array[i]);
    }
}

void test_goaway_payload_to_bytes_error(void)
{
    frame_header_t frame_header;

    frame_header.length = 16;
    frame_header.type = GOAWAY_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 0;

    goaway_payload_t goaway_payload;
    goaway_payload.last_stream_id = 30;
    goaway_payload.error_code = 1;
    uint8_t additional_debug_data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    goaway_payload.additional_debug_data = additional_debug_data;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    uint32_31_to_byte_array_fake.return_val = -1;
    uint32_to_byte_array_fake.return_val = -1;

    uint8_t byte_array[16];

    int rc = goaway_payload_to_bytes(&frame_header, &goaway_payload, byte_array);
    TEST_ASSERT_EQUAL(-1, rc);
    uint32_31_to_byte_array_fake.return_val = 0;
    rc = goaway_payload_to_bytes(&frame_header, &goaway_payload, byte_array);
    TEST_ASSERT_EQUAL(-1, rc);
}


void test_read_goaway_payload(void)
{
    /*expected*/
    frame_header_t expected_frame_header;

    expected_frame_header.type = GOAWAY_TYPE;
    expected_frame_header.flags = 0x0;
    expected_frame_header.stream_id = 0;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 16;

    goaway_payload_t expected_goaway_payload;
    expected_goaway_payload.last_stream_id = 30;
    expected_goaway_payload.error_code = 1;
    uint8_t additional_debug_data[] = { 1, 2, 3, 4, 5, 6, 7, 8 };
    expected_goaway_payload.additional_debug_data = additional_debug_data;


    /*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_fake.return_val = expected_goaway_payload.error_code;
    bytes_to_uint32_24_fake.return_val = expected_frame_header.length;
    bytes_to_uint32_31_fake.return_val = expected_goaway_payload.last_stream_id;


    //fill the buffer
    uint8_t read_buffer[] = { 0, 0, 0, 30,
                              0, 0, 0, 1,
                              1, 2, 3, 4, 5, 6, 7, 8 };

    /*result*/
    goaway_payload_t goaway_payload;
    uint8_t debug[30];
    goaway_payload.additional_debug_data = debug;
    int rc = read_goaway_payload(&expected_frame_header, (void *)&goaway_payload, read_buffer);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 16 bytes
    TEST_ASSERT_EQUAL(expected_goaway_payload.last_stream_id, goaway_payload.last_stream_id);
    TEST_ASSERT_EQUAL(expected_goaway_payload.error_code, goaway_payload.error_code);

    for (int i = 0; i < expected_frame_header.length - 8; i++) {
        TEST_ASSERT_EQUAL(expected_goaway_payload.additional_debug_data[i], goaway_payload.additional_debug_data[i]);
    }
}
int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_create_goaway_frame);
    UNIT_TEST(test_goaway_payload_to_bytes);
    UNIT_TEST(test_goaway_payload_to_bytes_error);
    UNIT_TEST(test_read_goaway_payload);

    return UNIT_TESTS_END();
}
