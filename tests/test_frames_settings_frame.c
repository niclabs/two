#include "unit.h"
#include "logging.h"
#include "frames/structs.h" //Common structs for frames
#include "settings_frame.h"
#include "fff.h"


// Include header definitions for file to test
// e.g #include "sock_non_blocking.h"
// Include header definitions for file to test
DEFINE_FFF_GLOBALS;

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t *);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t *);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t *);


/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(bytes_to_uint32)             \
    FAKE(bytes_to_uint16)             \
    FAKE(uint16_to_byte_array)        \
    FAKE(uint32_to_byte_array)        \

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


/* Mocks */
uint32_t bytes_to_uint32_custom_fake_num(uint8_t *bytes)
{
    return (uint32_t)bytes[3];
}

uint16_t bytes_to_uint16_custom_fake_num(uint8_t *bytes)
{
    return (uint16_t)bytes[1];
}

int uint32_to_byte_array_custom_fake_300(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 1;
    byte_array[3] = 44;
    return 0;
}

int uint16_to_byte_array_custom_fake_300(uint16_t num, uint8_t *byte_array)
{
    byte_array[0] = 1;
    byte_array[1] = 44;
    return 0;
}


void test_create_list_of_settings_pair(void)
{
    //TODO: not implemented yet.
    int count = 3;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++) {
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    settings_pair_t result_settings_pair[count];
    create_list_of_settings_pair(ids, values, count, result_settings_pair);

    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL(ids[i], result_settings_pair[i].identifier);
        TEST_ASSERT_EQUAL(values[i], result_settings_pair[i].value);
    }
}


void test_create_settings_frame(void)
{
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++) {
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }

    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];
    create_settings_frame(ids, values, count, &frame_header, &settings_payload, setting_pairs);

    TEST_ASSERT_EQUAL(count * 6, frame_header.length);
    TEST_ASSERT_EQUAL(0x0, frame_header.flags);
    TEST_ASSERT_EQUAL(0x4, frame_header.type);
    TEST_ASSERT_EQUAL(0x0, frame_header.reserved);
    TEST_ASSERT_EQUAL(0x0, frame_header.stream_id);

    TEST_ASSERT_EQUAL(count, settings_payload.count);

    settings_pair_t *setting_pairs_created = settings_payload.pairs;
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL(ids[i], setting_pairs_created[i].identifier);
        TEST_ASSERT_EQUAL(values[i], setting_pairs_created[i].value);
    }
}

void test_read_settings_payload(void)
{
    uint8_t bytes[] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1 };

    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++) {
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    /*settings_pair_t expected_setting_pairs[count];
       create_list_of_settings_pair(ids, values, count, expected_setting_pairs);
       settings_payload_t expected_settings_payload;
       expected_settings_payload.pairs = expected_setting_pairs;
       expected_settings_payload.count= count;
     */

    bytes_to_uint16_fake.custom_fake = bytes_to_uint16_custom_fake_num;
    bytes_to_uint32_fake.custom_fake = bytes_to_uint32_custom_fake_num;

    settings_payload_t result_settings_payload;
    settings_pair_t result_setting_pairs[count];
    result_settings_payload.pairs = result_setting_pairs;

    frame_header_t header;
    header.length = count * 6 + 1;

    int rc = read_settings_payload(&header, &result_settings_payload, bytes);
    TEST_ASSERT_EQUAL(-1, rc);

    header.length = count * 6 ;
    read_settings_payload(&header, &result_settings_payload, bytes);

    TEST_ASSERT_EQUAL(count, bytes_to_uint16_fake.call_count);
    TEST_ASSERT_EQUAL(count, bytes_to_uint32_fake.call_count);

    TEST_ASSERT_EQUAL(count, result_settings_payload.count);
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL(ids[i], result_setting_pairs[i].identifier);
        TEST_ASSERT_EQUAL(values[i], result_setting_pairs[i].value);
    }
}

void test_setting_to_bytes(void)
{
    uint16_t id = 300;
    uint32_t value = 300;

    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_300;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_300;
    settings_pair_t settings_pair = { id, value };
    uint8_t expected_bytes[6] = { 1, 44, 0, 0, 1, 44 };
    uint8_t byte_array[6];

    int rc = setting_to_bytes(&settings_pair, byte_array);

    TEST_ASSERT_EQUAL(6, rc);

    TEST_ASSERT_EQUAL(1, uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1, uint32_to_byte_array_fake.call_count);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], byte_array[i]);
    }
}

void test_create_settings_ack_frame(void)
{
    frame_t frame;
    frame_header_t frame_header;

    create_settings_ack_frame(&frame, &frame_header);

    TEST_ASSERT_EQUAL(SETTINGS_ACK_FLAG, frame.frame_header->flags);
    TEST_ASSERT_EQUAL(SETTINGS_TYPE, frame.frame_header->type);
    TEST_ASSERT_EQUAL(0, frame.frame_header->length);
    TEST_ASSERT_EQUAL(0, frame.frame_header->stream_id);
    TEST_ASSERT_EQUAL(0, frame.frame_header->reserved);
}



int main(void)
{
    UNIT_TESTS_BEGIN();

    // Call tests here
    UNIT_TEST(test_create_list_of_settings_pair);
    UNIT_TEST(test_create_settings_frame);
    UNIT_TEST(test_read_settings_payload);
    UNIT_TEST(test_setting_to_bytes);
    UNIT_TEST(test_create_settings_ack_frame);
    //TODO create settings_payload_to_bytes_test
    return UNIT_TESTS_END();
}
