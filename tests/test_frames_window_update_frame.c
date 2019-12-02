#include "unit.h"
#include "logging.h"

#include "unit.h"
#include "logging.h"
#include "frames/structs.h" //Common structs for frames
#include "window_update_frame.h"
#include "fff.h"


// Include header definitions for file to test
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t *);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t *);



/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(uint32_31_to_byte_array)     \


void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


/* Mocks */

int uint32_to_byte_array_custom_fake_num(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = (uint8_t)num;
    return 0;
}


/*Here begins the tests*/
void test_create_window_update_frame(void)
{
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    int window_size_increment = 30;
    uint32_t stream_id = 1;

    int rc = create_window_update_frame(&frame_header, &window_update_payload, window_size_increment, stream_id);

    TEST_ASSERT_EQUAL(0, rc);
    TEST_ASSERT_EQUAL(4, frame_header.length);
    TEST_ASSERT_EQUAL(WINDOW_UPDATE_TYPE, frame_header.type);
    TEST_ASSERT_EQUAL(0x0, frame_header.flags);
    TEST_ASSERT_EQUAL(stream_id, frame_header.stream_id);

    TEST_ASSERT_EQUAL(window_size_increment, window_update_payload.window_size_increment);
}


void test_window_update_payload_to_bytes(void)
{
    frame_header_t frame_header;

    frame_header.length = 4;
    frame_header.type = WINDOW_UPDATE_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 1;

    window_update_payload_t window_update_payload;
    window_update_payload.window_size_increment = 30;
    uint8_t byte_array[4];

    uint8_t expected_bytes[] = { 0, 0, 0, 30 };

    uint32_31_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;

    int rc = window_update_payload_to_bytes(&frame_header, &window_update_payload, byte_array);
    TEST_ASSERT_EQUAL(4, rc);
    TEST_ASSERT_EQUAL(4, frame_header.length);

    for (int i = 0; i < frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], byte_array[i]);
    }

}

void test_read_window_update_payload(void)
{
    uint8_t buff_read[] = { 0, 0, 0, 30 };
    frame_header_t frame_header;

    frame_header.length = 4;
    frame_header.type = WINDOW_UPDATE_TYPE;
    frame_header.flags = 0x0;
    frame_header.stream_id = 1;

    window_update_payload_t window_update_payload;

    bytes_to_uint32_31_fake.return_val = 30;
    int rc = read_window_update_payload(&frame_header, (void*) &window_update_payload, buff_read);

    TEST_ASSERT_EQUAL(4, rc);
    TEST_ASSERT_EQUAL(30, window_update_payload.window_size_increment);


}

int main(void)
{
    UNIT_TESTS_BEGIN();
    
    // Call tests here
    UNIT_TEST(test_create_window_update_frame);
    UNIT_TEST(test_window_update_payload_to_bytes);
    UNIT_TEST(test_read_window_update_payload);

    return UNIT_TESTS_END();
}
