//
// Created by maite on 07-05-19.
//

#include <errno.h>

#include "unit.h"
#include "frames.h"
#include "fff.h"


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, uint32_24_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t*);

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t*);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t*);

FAKE_VALUE_FUNC(int, append_byte_arrays,uint8_t*, uint8_t*, int, int);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)        \
    FAKE(uint32_24_to_byte_array)   \
    FAKE(uint32_31_to_byte_array)   \
    FAKE(uint32_to_byte_array)      \
    FAKE(uint16_to_byte_array)      \
    FAKE(bytes_to_uint32)           \
    FAKE(bytes_to_uint32_31)        \
    FAKE(bytes_to_uint32_24)        \
    FAKE(bytes_to_uint16)           \
    FAKE(append_byte_arrays)

void setUp(void) {
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

int uint32_24_to_byte_array_custom_fake_10(uint32_t num, uint8_t* byte_array){
        byte_array[0] = 0;
        byte_array[1] = 0;
        byte_array[2] = 10;
        return 0;
    }
int uint32_31_to_byte_array_custom_fake_2(uint32_t num, uint8_t* byte_array){
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = 2;
    return 0;
}

void test_frame_header_to_bytes(void) {
    uint32_t length = 0;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_10 ;//length
    uint8_t type = 0x1;
    uint8_t flags = 0x4;
    uint8_t reserved = 0x0;
    uint32_t stream_id = 2;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_2;//stream_id

    frame_header_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = {0,0,10,type,flags,0,0,0,2};

    TEST_ASSERT_EQUAL(9, frame_bytes_size);

    TEST_ASSERT_EQUAL(1,uint32_24_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1,uint32_31_to_byte_array_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[0],frame_bytes[0], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[1],frame_bytes[1], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[2],frame_bytes[2], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[3],frame_bytes[3],"wrong type");
    TEST_ASSERT_EQUAL(expected_bytes[4],frame_bytes[4]);
    TEST_ASSERT_EQUAL(expected_bytes[5],frame_bytes[5]);
    TEST_ASSERT_EQUAL(expected_bytes[6],frame_bytes[6]);
    TEST_ASSERT_EQUAL(expected_bytes[7],frame_bytes[7]);
    TEST_ASSERT_EQUAL(expected_bytes[8],frame_bytes[8]);
}


void test_frame_header_to_bytes_reserved(void) {
    uint32_t length = 0;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_10;//length
    uint8_t type = 0x1;
    uint8_t flags = 0x4;
    uint8_t reserved = 0x1;
    uint32_t stream_id = 2;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_2;//stream_id

    frame_header_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = {0,0,10,type,flags,128,0,0,2};

    TEST_ASSERT_EQUAL(9, frame_bytes_size);

    TEST_ASSERT_EQUAL(1,uint32_24_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1,uint32_31_to_byte_array_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[0],frame_bytes[0], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[1],frame_bytes[1], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[2],frame_bytes[2], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[3],frame_bytes[3],"wrong type");
    TEST_ASSERT_EQUAL(expected_bytes[4],frame_bytes[4]);
    TEST_ASSERT_EQUAL(expected_bytes[5],frame_bytes[5]);
    TEST_ASSERT_EQUAL(expected_bytes[6],frame_bytes[6]);
    TEST_ASSERT_EQUAL(expected_bytes[7],frame_bytes[7]);
    TEST_ASSERT_EQUAL(expected_bytes[8],frame_bytes[8]);
}



void test_bytes_to_frame_header(void){
    uint32_t length = 10;
    uint32_t stream_id = 2;
    uint8_t type = 0x1;
    uint8_t flags = 0x4;
    uint8_t bytes[9] = {0,0,10,type,flags,0,0,0,2};

    bytes_to_uint32_24_fake.return_val = length;
    bytes_to_uint32_31_fake.return_val = stream_id;

    frame_header_t decoder_frame_header;
    bytes_to_frame_header(bytes, 9, &decoder_frame_header);


    TEST_ASSERT_EQUAL_MESSAGE(length,decoder_frame_header.length, "wrong length.");
    TEST_ASSERT_EQUAL_MESSAGE(flags, decoder_frame_header.flags, "wrong flag.");
    TEST_ASSERT_EQUAL_MESSAGE(type, decoder_frame_header.type, "wrong type.");
    TEST_ASSERT_EQUAL_MESSAGE(stream_id, decoder_frame_header.stream_id, "wrong tream id.");
    TEST_ASSERT_EQUAL_MESSAGE(0, decoder_frame_header.reserved, "wrong Reserved bit.");
}


/*void test_bytes_to_frame_header(void){
    //TODO: not implemented yet.
}*/
void test_set_flag(void){
    uint8_t flag_byte = (uint8_t)0;
    uint8_t set_flag_1 = (uint8_t)0x1;
    uint8_t set_flag_2 = (uint8_t)0x4;
    uint8_t unset_flag = (uint8_t)0x8;

    flag_byte = set_flag(flag_byte, set_flag_1);
    flag_byte = set_flag(flag_byte, set_flag_2);

    TEST_ASSERT_EQUAL(1,is_flag_set(flag_byte,set_flag_1));
    TEST_ASSERT_EQUAL(1,is_flag_set(flag_byte,set_flag_2));
    TEST_ASSERT_EQUAL(0,is_flag_set(flag_byte,unset_flag));
}


void test_frame_to_bytes(void){
    //TODO: not implemented yet.
}
void test_create_list_of_settings_pair(void){
    //TODO: not implemented yet.
}
void test_create_settings_frame(void){
    //TODO: not implemented yet.
}
void test_setting_to_bytes(void){
    //TODO: not implemented yet.
}
void test_settings_frame_to_bytes(void){
    //TODO: not implemented yet.
}
void test_bytes_to_settings_payload(void){
    //TODO: not implemented yet.
}
void test_create_settings_ack_frame(void){
    //TODO: not implemented yet.
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_set_flag);
    //UNIT_TEST(test_frame_header_to_bytes_to_frame_header);
    UNIT_TEST(test_frame_header_to_bytes);
    UNIT_TEST(test_frame_header_to_bytes_reserved);
    UNIT_TEST(test_bytes_to_frame_header);
    return UNIT_TESTS_END();
}
