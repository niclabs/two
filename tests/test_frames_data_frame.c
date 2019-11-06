#include "unit.h"
#include "logging.h"
#include "frames/common.h" //Common structs for frames
#include "data_frame.h"
#include "fff.h"


// Include header definitions for file to test 
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);
FAKE_VALUE_FUNC(int, is_flag_set, uint8_t , uint8_t);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(buffer_copy)                 \
    FAKE(is_flag_set)

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

int is_flag_set_fake_custom(uint8_t flags, uint8_t queried_flag)
{
    if ((queried_flag & flags) > 0) {
        return 1;
    }
    return 0;
}

/*Here begins the tests*/
void test_create_data_frame(void)
{

    frame_header_t frame_header;
    data_payload_t data_payload;
    uint8_t data[10];
    uint8_t data_to_send[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    int length = 10;
    uint32_t stream_id = 1;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    int rc = create_data_frame(&frame_header, &data_payload, data, data_to_send, length, stream_id);

    TEST_ASSERT_EQUAL(0, rc);

    TEST_ASSERT_EQUAL(length, frame_header.length);
    TEST_ASSERT_EQUAL(DATA_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0x0, frame_header.flags);
    TEST_ASSERT_EQUAL(stream_id, frame_header.stream_id);
    for (int i = 0; i < length; i++) {
        TEST_ASSERT_EQUAL(data_to_send[i], data_payload.data[i]);
    }

}

void test_data_payload_to_bytes(void)
{
    frame_header_t frame_header;
    data_payload_t data_payload;
    int length = 10;
    uint32_t stream_id = 1;
    uint8_t data[10];
    uint8_t data_to_send[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    is_flag_set_fake.custom_fake = is_flag_set_fake_custom;
    create_data_frame(&frame_header, &data_payload, data, data_to_send, length, stream_id);
    uint8_t byte_array[30];
    int rc = data_payload_to_bytes(&frame_header, &data_payload, byte_array);

    TEST_ASSERT_EQUAL(length, rc);
    for (int i = 0; i < frame_header.length; i++) {
        TEST_ASSERT_EQUAL(data[i], byte_array[i]);
    }

}

void test_read_data_payload(void)
{
    uint8_t buff_read[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };
    frame_header_t frame_header;

    frame_header.length = 10;
    frame_header.type = DATA_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 1;

    data_payload_t data_payload;
    uint8_t data[10];
    data_payload.data = data;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

    //int read_data_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes);

    int rc = read_data_payload(&frame_header, (void*) &data_payload, buff_read);
    TEST_ASSERT_EQUAL(frame_header.length, rc);
    for (int i = 0; i < frame_header.length; i++) {
        TEST_ASSERT_EQUAL(buff_read[i], data_payload.data[i]);
    }

}

int main(void)
{
    UNIT_TESTS_BEGIN();
    
    // Call tests here
    UNIT_TEST(test_create_data_frame);
    UNIT_TEST(test_data_payload_to_bytes);
    UNIT_TEST(test_read_data_payload);

    return UNIT_TESTS_END();
}
