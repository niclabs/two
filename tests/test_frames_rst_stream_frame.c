//
// Created by gabriel on 19-11-19.
//

#include "unit.h"
#include "logging.h"
#include "frames/common.h" //Common structs for frames
#include "rst_stream_frame.h"
#include "fff.h"


// Include header definitions for file to test
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(buffer_copy)                 \
    FAKE(uint32_to_byte_array)        \
    FAKE(bytes_to_uint32)

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

uint32_t bytes_to_uint32_custom_fake(uint8_t *bytes){
    uint32_t number = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return number;
}


/*Here begin the tests*/
void test_create_rst_stream_frame(void)
{
    frame_header_t frame_header;
    rst_stream_payload_t rst_stream_payload;
    uint32_t last_stream_id = 30;
    uint32_t error_code = 1;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_rst_stream_frame(&frame_header,
                                 &rst_stream_payload,
                                 last_stream_id,
                                 error_code);

    TEST_ASSERT_EQUAL(4, frame_header.length);
    TEST_ASSERT_EQUAL(RST_STREAM_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0x0, frame_header.flags);
    TEST_ASSERT_EQUAL(30, frame_header.stream_id);

    TEST_ASSERT_EQUAL(error_code, rst_stream_payload.error_code);
}


void test_rst_stream_payload_to_bytes(void)
{
    frame_header_t frame_header;

    frame_header.length = 4;
    frame_header.type = RST_STREAM_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 2;

    rst_stream_payload_t rst_stream_payload;
    rst_stream_payload.error_code = 1;


    uint8_t expected_bytes[] = { 0, 0, 0, 1};

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint8_t byte_array[16];

    int rc = rst_stream_payload_to_bytes(&frame_header, &rst_stream_payload, byte_array);
    TEST_ASSERT_EQUAL(4, rc);
    TEST_ASSERT_EQUAL(4, frame_header.length);

    for (int i = 0; i < frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], byte_array[i]);
    }
}


void test_read_rst_stream_payload(void)
{
/*expected*/

    frame_header_t expected_frame_header;

    expected_frame_header.type = RST_STREAM_TYPE;
    expected_frame_header.flags = 0;
    expected_frame_header.stream_id = 1;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 4;
    rst_stream_payload_t expected_rst_stream_payload = { .error_code = 3 };
    //uint8_t expected_padding[32] = {};
    //expected_headers_payload.padding = expected_padding;

/*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_fake.custom_fake = bytes_to_uint32_custom_fake;

    //fill the buffer
    uint8_t read_buffer[4] = { 0, 0, 0, 3 };/*payload*/
/*result*/
    rst_stream_payload_t rst_stream_payload;

//uint8_t padding[32]; //not implemented yet!
//int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)

    int rc = read_rst_stream_payload(&expected_frame_header, (void *)&rst_stream_payload, read_buffer);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 8 bytes
    TEST_ASSERT_EQUAL(expected_rst_stream_payload.error_code, rst_stream_payload.error_code);

}


int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_create_rst_stream_frame);
    UNIT_TEST(test_rst_stream_payload_to_bytes);

    UNIT_TEST(test_read_rst_stream_payload);

    return UNIT_TESTS_END();
}
