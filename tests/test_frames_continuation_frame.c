#include "unit.h"
#include "logging.h"
#include "frames/common.h" //Common structs for frames
#include "continuation_frame.h"
#include "fff.h"

// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(buffer_copy)

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

void test_create_continuation_frame(void)
{

    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 30;
    frame_header_t frame_header;
    continuation_payload_t continuation_payload;
    uint8_t header_block_fragment[6];

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;


    int rc = create_continuation_frame(headers_block, headers_block_size, stream_id, &frame_header,
                                       &continuation_payload, header_block_fragment);
    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(stream_id, frame_header.stream_id);
    TEST_ASSERT_EQUAL(CONTINUATION_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0, frame_header.flags);
    TEST_ASSERT_EQUAL(6, frame_header.length);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(headers_block[i], continuation_payload.header_block_fragment[i]);
    }

}

void test_continuation_payload_to_bytes(void)
{
    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 1;
    frame_header_t frame_header;
    continuation_payload_t continuation_payload;
    uint8_t header_block_fragment[6];

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    create_continuation_frame(headers_block, headers_block_size, stream_id, &frame_header, &continuation_payload,
                              header_block_fragment);
    uint8_t byte_array[6];
    int rc = continuation_payload_to_bytes(&frame_header, &continuation_payload, byte_array);
    TEST_ASSERT_EQUAL(6, rc);
    uint8_t expected_byte_array[6] = { 1, 2, 3, 4, 5, 6 };
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_byte_array[i], byte_array[i]);
    }
}


void test_read_continuation_payload(void)
{

    /*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    /*expected*/
    frame_header_t expected_frame_header;
    expected_frame_header.type = 0x9;
    expected_frame_header.flags = 0x4;
    expected_frame_header.stream_id = 1;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 8;
    uint8_t expected_headers_block_fragment[64] = { 0, 12, 34, 5, 234, 7, 34, 98 };

    //fill the buffer
    uint8_t read_buffer[20] = { 0, 12, 34, 5, 234, 7, 34, 98, 9, 0, 0, 1, 12 /*payload*/ };

    /*result*/
    continuation_payload_t continuation_payload;
    //int read_continuation_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)

    int rc = read_continuation_payload(&expected_frame_header, (void*) &continuation_payload, read_buffer);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 18 bytes
    for (int i = 0; i < expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_headers_block_fragment[i], continuation_payload.header_block_fragment[i]);
    }
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_create_continuation_frame);
    UNIT_TEST(test_read_continuation_payload);
    UNIT_TEST(test_continuation_payload_to_bytes);

    return UNIT_TESTS_END();
}
