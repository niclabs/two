//
// Created by maite on 07-05-19.
//

#include <errno.h>

#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "utils.h"
#include "logging.h"


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, uint32_24_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t*);

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t*);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t*);

FAKE_VALUE_FUNC(int, append_byte_arrays, uint8_t*, uint8_t*, uint8_t*, int, int);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t*, uint8_t*, int);


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
    FAKE(append_byte_arrays)        \
    FAKE(buffer_copy)

void setUp(void) {
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

/* Mocks */
int buffer_copy_fake_custom(uint8_t* dest, uint8_t* orig, int size){
    for(int i = 0; i< size; i++){
        dest[i] = orig[i];
    }
    return size;
}

void test_read_headers_payload(void){


    /*expected*/
    frame_header_t expected_frame_header;
    expected_frame_header.type = 0x1;
    expected_frame_header.flags = set_flag(0x0, 0x4);
    expected_frame_header.stream_id = 1;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 9;
    headers_payload_t expected_headers_payload;
    uint8_t expected_headers_block_fragment[64] = {0,12,34,5,234,7,34,98,9};
    //uint8_t expected_padding[32] = {};
    expected_headers_payload.header_block_fragment = expected_headers_block_fragment;
    //expected_headers_payload.padding = expected_padding;

    /*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_24_fake.return_val = expected_frame_header.length;
    bytes_to_uint32_31_fake.return_val = expected_frame_header.stream_id;


    //fill the buffer
    uint8_t read_buffer[20] = {0,12,34,5,234,7,34,98,9,0,0,1,12,3/*payload*/};

    /*result*/
    headers_payload_t headers_payload;
    uint8_t headers_block_fragment[64];
    uint8_t padding[32];
    int rc = read_headers_payload(read_buffer, &expected_frame_header, &headers_payload, headers_block_fragment, padding);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 9 bytes
    for(int i =0; i<expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_headers_payload.header_block_fragment[i], headers_payload.header_block_fragment[i]);
    }
}
//int get_header_block_fragment_size(frame_header_t* frame_header, headers_payload_t *headers_payload);
//int receive_header_block(uint8_t* header_block_fragments, int header_block_fragments_pointer, table_pair_t* header_list, uint8_t table_index);

void test_read_continuation_payload(void){

    /*mocks*/
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

//int read_continuation_payload(uint8_t* buff_read, frame_header_t* frame_header, continuation_payload_t* continuation_payload, uint8_t * continuation_block_fragment);

    /*expected*/
    frame_header_t expected_frame_header;
    expected_frame_header.type = 0x9;
    expected_frame_header.flags = 0x4;
    expected_frame_header.stream_id = 1;
    expected_frame_header.reserved = 0;
    expected_frame_header.length = 8;
    uint8_t expected_headers_block_fragment[64] = {0,12,34,5,234,7,34,98};



    //fill the buffer
    uint8_t read_buffer[20] = {0,12,34,5,234,7,34,98,9,0,0,1,12/*payload*/};

    /*result*/
    continuation_payload_t continuation_payload;
    uint8_t headers_block_fragment[64];
    int rc = read_continuation_payload(read_buffer, &expected_frame_header, &continuation_payload, headers_block_fragment);

    TEST_ASSERT_EQUAL(expected_frame_header.length,rc);//se leyeron 18 bytes
    for(int i =0; i<expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_headers_block_fragment[i], continuation_payload.header_block_fragment[i]);
    }
}



void test_set_flag(void){
    uint8_t flag_byte = (uint8_t)0;
    uint8_t set_flag_1 = (uint8_t)0x1;
    uint8_t set_flag_2 = (uint8_t)0x4;

    flag_byte = set_flag(flag_byte, set_flag_1);

    TEST_ASSERT_EQUAL(flag_byte,set_flag_1);

    flag_byte = set_flag(flag_byte, set_flag_2);

    TEST_ASSERT_EQUAL(flag_byte,set_flag_1|set_flag_2);
}

void test_is_flag_set(void){
    uint8_t flag_byte = (uint8_t)0;
    uint8_t set_flag_1 = (uint8_t)0x1;
    uint8_t set_flag_2 = (uint8_t)0x4;
    uint8_t unset_flag = (uint8_t)0x8;

    flag_byte = set_flag(flag_byte, set_flag_1);
    TEST_ASSERT_EQUAL(1,is_flag_set(flag_byte,set_flag_1));
    flag_byte = set_flag(flag_byte, set_flag_2);
    TEST_ASSERT_EQUAL(1,is_flag_set(flag_byte,set_flag_1));
    TEST_ASSERT_EQUAL(1,is_flag_set(flag_byte,set_flag_2));
    TEST_ASSERT_EQUAL(0,is_flag_set(flag_byte,unset_flag));
}



int uint32_24_to_byte_array_custom_fake_6(uint32_t num, uint8_t* byte_array){
        byte_array[0] = 0;
        byte_array[1] = 0;
        byte_array[2] = 6;
        return 0;
    }
int uint32_31_to_byte_array_custom_fake_0(uint32_t num, uint8_t* byte_array){
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = 0;
    return 0;
}
void test_frame_header_to_bytes(void) {
    uint32_t length = 6;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6 ;//length
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t reserved = 0x0;
    uint32_t stream_id = 0;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;//stream_id

    frame_header_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = {0,0,6,type,flags,0,0,0,0};

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
    uint32_t length = 6;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;//length
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t reserved = 0x1;
    uint32_t stream_id = 0;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;//stream_id

    frame_header_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = {0,0,6,type,flags,128,0,0,0};

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
    uint32_t length = 6;
    uint32_t stream_id = 0;
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t bytes[9] = {0,0,6,type,flags,0,0,0,0};

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

void test_create_list_of_settings_pair(void){
    //TODO: not implemented yet.
    int count = 3;
    uint16_t ids[count];
    uint32_t values[count];
    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    settings_pair_t  result_settings_pair[count];
    create_list_of_settings_pair(ids, values, count, result_settings_pair);

    for (int i = 0; i < count; i++){
        TEST_ASSERT_EQUAL(ids[i],result_settings_pair[i].identifier);
        TEST_ASSERT_EQUAL(values[i],result_settings_pair[i].value);
    }
}
void test_create_settings_frame(void){
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];
    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    frame_t frame;
    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];
    create_settings_frame(ids, values, count, &frame, &frame_header, &settings_payload, setting_pairs);

    TEST_ASSERT_EQUAL(count*6,frame.frame_header->length);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->flags);
    TEST_ASSERT_EQUAL(0x4, frame.frame_header->type);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->reserved);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->stream_id);

    settings_payload_t *settings_payload_created = (settings_payload_t*)frame.payload;

    TEST_ASSERT_EQUAL(count, settings_payload_created->count);

    settings_pair_t *setting_pairs_created = settings_payload.pairs;
    for (int i = 0; i < count; i++){
        TEST_ASSERT_EQUAL(ids[i],setting_pairs_created[i].identifier);
        TEST_ASSERT_EQUAL(values[i],setting_pairs_created[i].value);
    }
}


int uint32_to_byte_array_custom_fake_300(uint32_t num, uint8_t* byte_array){
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 1;
    byte_array[3] = 44;
    return 0;
}
int uint16_to_byte_array_custom_fake_300(uint16_t num, uint8_t* byte_array){
    byte_array[0] = 1;
    byte_array[1] = 44;
    return 0;
}

void test_setting_to_bytes(void){
    uint16_t id = 300;
    uint32_t value = 300;

    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_300;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_300;
    settings_pair_t  settings_pair = {id, value};
    uint8_t expected_bytes[6] = {1, 44, 0, 0, 1, 44};
    uint8_t byte_array[6];

    setting_to_bytes(&settings_pair, byte_array);

    TEST_ASSERT_EQUAL(1, uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1, uint32_to_byte_array_fake.call_count);
    for (int i = 0; i < 6; i++){
        TEST_ASSERT_EQUAL(expected_bytes[i],byte_array[i]);
    }
}


int uint32_to_byte_array_custom_fake_num(uint32_t num, uint8_t* byte_array){
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = (uint8_t)num;
    return 0;
}
int uint16_to_byte_array_custom_fake_num(uint16_t num, uint8_t* byte_array){
    byte_array[0] = 0;
    byte_array[1] = (uint8_t)num;
    return 0;
}

void test_settings_frame_to_bytes(void){
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];
    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    settings_pair_t setting_pairs[count];
    create_list_of_settings_pair(ids, values, count, setting_pairs);
    settings_payload_t settings_payload;
    settings_payload.pairs = setting_pairs;
    settings_payload.count= count;

    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;

    uint8_t byte_array[6*count];
    settings_frame_to_bytes(&settings_payload, count, byte_array);

    uint8_t expected_bytes[] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1};
    TEST_ASSERT_EQUAL(count, uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(count, uint32_to_byte_array_fake.call_count);
    for (int i = 0; i < 6; i++){
        TEST_ASSERT_EQUAL(expected_bytes[i],byte_array[i]);
    }
}

uint16_t bytes_to_uint16_custom_fake_num(uint8_t * bytes){
        return (uint16_t)bytes[1];
    }

uint32_t bytes_to_uint32_custom_fake_num(uint8_t * bytes){
    return (uint32_t)bytes[3];
}

void test_bytes_to_settings_payload(void){
    uint8_t bytes[] = {0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1};

    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];
    for (int i = 0; i < count; i++){
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
    bytes_to_settings_payload(bytes, count*6, &result_settings_payload, result_setting_pairs);



    TEST_ASSERT_EQUAL(count, bytes_to_uint16_fake.call_count);
    TEST_ASSERT_EQUAL(count, bytes_to_uint32_fake.call_count);

    TEST_ASSERT_EQUAL(count, result_settings_payload.count);
    for (int i = 0; i < count; i++){
        TEST_ASSERT_EQUAL(ids[i],result_setting_pairs[i].identifier);
        TEST_ASSERT_EQUAL(values[i],result_setting_pairs[i].value);
    }


}
void test_create_settings_ack_frame(void){
    frame_t frame;
    frame_header_t frame_header;
    create_settings_ack_frame(&frame, &frame_header);

    TEST_ASSERT_EQUAL(SETTINGS_ACK_FLAG,frame.frame_header->flags);
    TEST_ASSERT_EQUAL(SETTINGS_TYPE,frame.frame_header->type);
    TEST_ASSERT_EQUAL(0,frame.frame_header->length);
    TEST_ASSERT_EQUAL(0,frame.frame_header->stream_id);
    TEST_ASSERT_EQUAL(0,frame.frame_header->reserved);
}

int append_byte_arrays_custom_fake(uint8_t *dest, uint8_t *array1, uint8_t *array2, int size1, int size2){

    for(uint8_t i = 0; i < size1; i++){
        dest[i]=array1[i];
    }
    for(uint8_t i = 0; i < size2; i++){
        dest[size1+i]=array2[i];
    }
    //memcpy(total, array1, sizeof(array1));
    //memcpy(total+sizeof(array1), array2, sizeof(array2));
    return size1 + size2;
}



void test_frame_to_bytes(void){
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];
    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    frame_t frame;
    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];
    create_settings_frame(ids, values, count, &frame, &frame_header, &settings_payload, setting_pairs);


    uint8_t expected_bytes[] = {0,0,6, SETTINGS_TYPE,0, 0,0,0,0,  0,0, 0,0,0,0,  0,1, 0,0,0,1};


    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;
    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;


    uint8_t result_bytes[9+count*6];
    int size = frame_to_bytes(&frame, result_bytes);


    TEST_ASSERT_EQUAL(1,uint32_24_to_byte_array_fake.call_count);//length
    TEST_ASSERT_EQUAL(1,uint32_31_to_byte_array_fake.call_count);//stream_id
    TEST_ASSERT_EQUAL(count,uint32_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(count,uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(9+count*6,size);

    for (int i = 0; i < size; i++){
        TEST_ASSERT_EQUAL(expected_bytes[i],result_bytes[i]);
    }
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_set_flag);
    UNIT_TEST(test_is_flag_set);
    UNIT_TEST(test_frame_header_to_bytes);
    UNIT_TEST(test_frame_header_to_bytes_reserved);
    UNIT_TEST(test_bytes_to_frame_header);

    UNIT_TEST(test_create_list_of_settings_pair);
    UNIT_TEST(test_create_settings_frame);
    UNIT_TEST(test_setting_to_bytes);
    UNIT_TEST(test_settings_frame_to_bytes);
    UNIT_TEST(test_bytes_to_settings_payload);

    UNIT_TEST(test_create_settings_ack_frame);
    UNIT_TEST(test_frame_to_bytes);
    UNIT_TEST(test_read_headers_payload);
    UNIT_TEST(test_read_continuation_payload);

    return UNIT_TESTS_END();
}
