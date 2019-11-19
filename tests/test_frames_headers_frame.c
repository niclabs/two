#include "unit.h"
#include "logging.h"
#include "frames/common.h" //Common structs for frames
#include "headers_frame.h"
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


void test_create_headers_frame(void)
{
    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 1;
    frame_header_t frame_header;
    headers_payload_t headers_payload;
    uint8_t header_block_fragment[6];

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_headers_frame(headers_block, headers_block_size, stream_id, &frame_header, &headers_payload, header_block_fragment);

    TEST_ASSERT_EQUAL(1, frame_header.stream_id);
    TEST_ASSERT_EQUAL(HEADERS_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0, frame_header.flags);
    TEST_ASSERT_EQUAL(6, frame_header.length);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(headers_block[i], headers_payload.header_block_fragment[i]);
    }
}

void test_headers_payload_to_bytes(void)
{
    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 1;
    frame_header_t frame_header;
    headers_payload_t headers_payload;
    uint8_t header_block_fragment[6];

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_headers_frame(headers_block, headers_block_size, stream_id, &frame_header, &headers_payload, header_block_fragment);
    uint8_t byte_array[6];
    int rc = headers_payload_to_bytes(&frame_header, &headers_payload, byte_array);
    TEST_ASSERT_EQUAL(6, rc);
    uint8_t expected_byte_array[6] = { 1, 2, 3, 4, 5, 6 };
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_byte_array[i], byte_array[i]);
    }
}


void test_read_headers_payload(void)
{


    /*expected*/
    frame_header_t expected_frame_header;

    expected_frame_header.type = 0x1;
    expected_frame_header.flags = set_flag_fake_custom(0x0, 0x4);
    expected_frame_header.stream_id = 1;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 9;
    headers_payload_t expected_headers_payload;
    uint8_t expected_headers_block_fragment[64] = { 0, 12, 34, 5, 234, 7, 34, 98, 9 };
    //uint8_t expected_padding[32] = {};
    expected_headers_payload.header_block_fragment = expected_headers_block_fragment;
    //expected_headers_payload.padding = expected_padding;

    /*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_24_fake.return_val = expected_frame_header.length;
    bytes_to_uint32_31_fake.return_val = expected_frame_header.stream_id;


    //fill the buffer
    uint8_t read_buffer[20] = { 0, 12, 34, 5, 234, 7, 34, 98, 9, 0, 0, 1, 12, 3 /*payload*/ };

    /*result*/
    headers_payload_t headers_payload;
    uint8_t headers_block_fragment[64];
    headers_payload.header_block_fragment = headers_block_fragment;
    //uint8_t padding[32]; //not implemented yet!
    //int read_headers_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)

    int rc = read_headers_payload(&expected_frame_header, (void*) &headers_payload, read_buffer);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 9 bytes
    for (int i = 0; i < expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_headers_payload.header_block_fragment[i], headers_payload.header_block_fragment[i]);
    }
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    
    // Call tests here
    UNIT_TEST(test_read_headers_payload);
    UNIT_TEST(test_create_headers_frame);
    UNIT_TEST(test_headers_payload_to_bytes);

    return UNIT_TESTS_END();
}
