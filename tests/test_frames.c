//
// Created by maite on 07-05-19.
//

#include <errno.h>

#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "logging.h"
#include "hpack.h"
#include "headers.h"


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, uint32_24_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t *);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t *);

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t *);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t *);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t *);

FAKE_VALUE_FUNC(int, append_byte_arrays, uint8_t *, uint8_t *, uint8_t *, int, int);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t *, uint8_t *, int);

FAKE_VALUE_FUNC(int, encode, hpack_states_t *, char *, char *,  uint8_t *);
FAKE_VALUE_FUNC(int, decode_header_block, hpack_states_t *, uint8_t *, uint8_t, headers_t *);

// Headers functions
FAKE_VALUE_FUNC(int, headers_count, headers_t *);
FAKE_VALUE_FUNC(char *, headers_get_name_from_index, headers_t *, int);
FAKE_VALUE_FUNC(char *, headers_get_value_from_index, headers_t *, int);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)          \
    FAKE(uint32_24_to_byte_array)     \
    FAKE(uint32_31_to_byte_array)     \
    FAKE(uint32_to_byte_array)        \
    FAKE(uint16_to_byte_array)        \
    FAKE(bytes_to_uint32)             \
    FAKE(bytes_to_uint32_31)          \
    FAKE(bytes_to_uint32_24)          \
    FAKE(bytes_to_uint16)             \
    FAKE(append_byte_arrays)          \
    FAKE(buffer_copy)                 \
    FAKE(encode)                      \
    FAKE(decode_header_block)         \
    FAKE(headers_count)               \
    FAKE(headers_get_name_from_index) \
    FAKE(headers_get_value_from_index)

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

void test_read_headers_payload(void)
{


    /*expected*/
    frame_header_t expected_frame_header;

    expected_frame_header.type = 0x1;
    expected_frame_header.flags = set_flag(0x0, 0x4);
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
    //uint8_t padding[32]; //not implemented yet!
    int rc = read_headers_payload(read_buffer, &expected_frame_header, &headers_payload, headers_block_fragment, NULL);

    TEST_ASSERT_EQUAL(expected_frame_header.length, rc);//se leyeron 9 bytes
    for (int i = 0; i < expected_frame_header.length; i++) {
        TEST_ASSERT_EQUAL(expected_headers_payload.header_block_fragment[i], headers_payload.header_block_fragment[i]);
    }
}
//int get_header_block_fragment_size(frame_header_t* frame_header, headers_payload_t *headers_payload);
//int receive_header_block(uint8_t* header_block_fragments, int header_block_fragments_pointer, table_pair_t* header_list, uint8_t table_index);


void test_set_flag(void)
{
    uint8_t flag_byte = (uint8_t)0;
    uint8_t set_flag_1 = (uint8_t)0x1;
    uint8_t set_flag_2 = (uint8_t)0x4;

    flag_byte = set_flag(flag_byte, set_flag_1);

    TEST_ASSERT_EQUAL(flag_byte, set_flag_1);

    flag_byte = set_flag(flag_byte, set_flag_2);

    TEST_ASSERT_EQUAL(flag_byte, set_flag_1 | set_flag_2);
}

void test_is_flag_set(void)
{
    uint8_t flag_byte = (uint8_t)0;
    uint8_t set_flag_1 = (uint8_t)0x1;
    uint8_t set_flag_2 = (uint8_t)0x4;
    uint8_t unset_flag = (uint8_t)0x8;

    flag_byte = set_flag(flag_byte, set_flag_1);
    TEST_ASSERT_EQUAL(1, is_flag_set(flag_byte, set_flag_1));
    flag_byte = set_flag(flag_byte, set_flag_2);
    TEST_ASSERT_EQUAL(1, is_flag_set(flag_byte, set_flag_1));
    TEST_ASSERT_EQUAL(1, is_flag_set(flag_byte, set_flag_2));
    TEST_ASSERT_EQUAL(0, is_flag_set(flag_byte, unset_flag));
}




int uint32_24_to_byte_array_custom_fake_6(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 6;
    return 0;
}
int uint32_31_to_byte_array_custom_fake_0(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = 0;
    return 0;
}

int uint32_31_to_byte_array_custom_fake_1(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = 1;
    return 1;
}
void test_frame_header_to_bytes(void)
{
    uint32_t length = 6;

    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6; //length
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t reserved = 0x0;
    uint32_t stream_id = 0;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;//stream_id

    frame_header_t frame_header = { length, type, flags, reserved, stream_id };
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = { 0, 0, 6, type, flags, 0, 0, 0, 0 };

    TEST_ASSERT_EQUAL(9, frame_bytes_size);

    TEST_ASSERT_EQUAL(1, uint32_24_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1, uint32_31_to_byte_array_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[0], frame_bytes[0], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[1], frame_bytes[1], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[2], frame_bytes[2], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[3], frame_bytes[3], "wrong type");
    TEST_ASSERT_EQUAL(expected_bytes[4], frame_bytes[4]);
    TEST_ASSERT_EQUAL(expected_bytes[5], frame_bytes[5]);
    TEST_ASSERT_EQUAL(expected_bytes[6], frame_bytes[6]);
    TEST_ASSERT_EQUAL(expected_bytes[7], frame_bytes[7]);
    TEST_ASSERT_EQUAL(expected_bytes[8], frame_bytes[8]);
}


void test_frame_header_to_bytes_reserved(void)
{
    uint32_t length = 6;

    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;//length
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t reserved = 0x1;
    uint32_t stream_id = 0;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;//stream_id

    frame_header_t frame_header = { length, type, flags, reserved, stream_id };
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    uint8_t expected_bytes[9] = { 0, 0, 6, type, flags, 128, 0, 0, 0 };

    TEST_ASSERT_EQUAL(9, frame_bytes_size);

    TEST_ASSERT_EQUAL(1, uint32_24_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(1, uint32_31_to_byte_array_fake.call_count);

    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[0], frame_bytes[0], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[1], frame_bytes[1], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[2], frame_bytes[2], "wrong length");
    TEST_ASSERT_EQUAL_MESSAGE(expected_bytes[3], frame_bytes[3], "wrong type");
    TEST_ASSERT_EQUAL(expected_bytes[4], frame_bytes[4]);
    TEST_ASSERT_EQUAL(expected_bytes[5], frame_bytes[5]);
    TEST_ASSERT_EQUAL(expected_bytes[6], frame_bytes[6]);
    TEST_ASSERT_EQUAL(expected_bytes[7], frame_bytes[7]);
    TEST_ASSERT_EQUAL(expected_bytes[8], frame_bytes[8]);
}

void test_bytes_to_frame_header(void)
{
    uint32_t length = 6;
    uint32_t stream_id = 0;
    uint8_t type = 0x4;
    uint8_t flags = 0x0;
    uint8_t bytes[9] = { 0, 0, 6, type, flags, 0, 0, 0, 0 };


    frame_header_t decoder_frame_header;
   

    bytes_to_uint32_24_fake.return_val = length;
    bytes_to_uint32_31_fake.return_val = stream_id;


    bytes_to_frame_header(bytes, &decoder_frame_header);

    TEST_ASSERT_EQUAL_MESSAGE(length, decoder_frame_header.length, "wrong length.");
    TEST_ASSERT_EQUAL_MESSAGE(flags, decoder_frame_header.flags, "wrong flag.");
    TEST_ASSERT_EQUAL_MESSAGE(type, decoder_frame_header.type, "wrong type.");
    TEST_ASSERT_EQUAL_MESSAGE(stream_id, decoder_frame_header.stream_id, "wrong tream id.");
    TEST_ASSERT_EQUAL_MESSAGE(0, decoder_frame_header.reserved, "wrong Reserved bit.");
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
    frame_t frame;
    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];
    create_settings_frame(ids, values, count, &frame, &frame_header, &settings_payload, setting_pairs);

    TEST_ASSERT_EQUAL(count * 6, frame.frame_header->length);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->flags);
    TEST_ASSERT_EQUAL(0x4, frame.frame_header->type);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->reserved);
    TEST_ASSERT_EQUAL(0x0, frame.frame_header->stream_id);

    settings_payload_t *settings_payload_created = (settings_payload_t *)frame.payload;

    TEST_ASSERT_EQUAL(count, settings_payload_created->count);

    settings_pair_t *setting_pairs_created = settings_payload.pairs;
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL(ids[i], setting_pairs_created[i].identifier);
        TEST_ASSERT_EQUAL(values[i], setting_pairs_created[i].value);
    }
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


int uint32_to_byte_array_custom_fake_num(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = 0;
    byte_array[3] = (uint8_t)num;
    return 0;
}

int uint32_24_to_byte_array_custom_fake_num(uint32_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = 0;
    byte_array[2] = (uint8_t)num;
    return 0;
}


int uint16_to_byte_array_custom_fake_num(uint16_t num, uint8_t *byte_array)
{
    byte_array[0] = 0;
    byte_array[1] = (uint8_t)num;
    return 0;
}

void test_settings_frame_to_bytes(void)
{
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++) {
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    settings_pair_t setting_pairs[count];
    create_list_of_settings_pair(ids, values, count, setting_pairs);
    settings_payload_t settings_payload;
    settings_payload.pairs = setting_pairs;
    settings_payload.count = count;

    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;

    uint8_t byte_array[6 * count];
    int rc = settings_payload_to_bytes(&settings_payload, count, byte_array);
    TEST_ASSERT_EQUAL(6 * count, rc);
    uint8_t expected_bytes[] = { 0, 0, 0, 0, 0, 0, 0, 1, 0, 0, 0, 1 };
    TEST_ASSERT_EQUAL(count, uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(count, uint32_to_byte_array_fake.call_count);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], byte_array[i]);
    }
}

uint16_t bytes_to_uint16_custom_fake_num(uint8_t *bytes)
{
    return (uint16_t)bytes[1];
}

uint32_t bytes_to_uint32_custom_fake_num(uint8_t *bytes)
{
    return (uint32_t)bytes[3];
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
    frame_header_t header;
    header.length = count * 6 + 1;

    int rc = read_settings_payload(bytes, &header, &result_settings_payload, result_setting_pairs);
    TEST_ASSERT_EQUAL(-1, rc);

    header.length = count * 6 ;
    read_settings_payload(bytes, &header, &result_settings_payload, result_setting_pairs);

    TEST_ASSERT_EQUAL(count, bytes_to_uint16_fake.call_count);
    TEST_ASSERT_EQUAL(count, bytes_to_uint32_fake.call_count);

    TEST_ASSERT_EQUAL(count, result_settings_payload.count);
    for (int i = 0; i < count; i++) {
        TEST_ASSERT_EQUAL(ids[i], result_setting_pairs[i].identifier);
        TEST_ASSERT_EQUAL(values[i], result_setting_pairs[i].value);
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

int append_byte_arrays_custom_fake(uint8_t *dest, uint8_t *array1, uint8_t *array2, int size1, int size2)
{

    for (uint8_t i = 0; i < size1; i++) {
        dest[i] = array1[i];
    }
    for (uint8_t i = 0; i < size2; i++) {
        dest[size1 + i] = array2[i];
    }
    //memcpy(total, array1, sizeof(array1));
    //memcpy(total+sizeof(array1), array2, sizeof(array2));
    return size1 + size2;
}



void test_frame_to_bytes_settings(void)
{
    int count = 2;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++) {
        ids[i] = (uint16_t)i;
        values[i] = (uint32_t)i;
    }
    frame_t frame;
    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];
    create_settings_frame(ids, values, count, &frame, &frame_header, &settings_payload, setting_pairs);


    uint8_t expected_bytes[] = { 0, 0, 6, SETTINGS_TYPE, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0,  0, 1, 0, 0, 0, 1 };


    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_0;
    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;


    uint8_t result_bytes[9 + count * 6];
    int size = frame_to_bytes(&frame, result_bytes);


    TEST_ASSERT_EQUAL(1, uint32_24_to_byte_array_fake.call_count);  //length
    TEST_ASSERT_EQUAL(1, uint32_31_to_byte_array_fake.call_count);  //stream_id
    TEST_ASSERT_EQUAL(count, uint32_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(count, uint16_to_byte_array_fake.call_count);
    TEST_ASSERT_EQUAL(9 + count * 6, size);

    for (int i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], result_bytes[i]);
    }
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

    int rc = create_headers_frame(headers_block, headers_block_size, stream_id, &frame_header, &headers_payload, header_block_fragment);

    TEST_ASSERT_EQUAL(0, rc);
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

void test_frame_to_bytes_headers(void)
{
    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 1;
    frame_header_t frame_header;
    headers_payload_t headers_payload;
    uint8_t header_block_fragment[6];


    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_1;
    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;


    create_headers_frame(headers_block, headers_block_size, stream_id, &frame_header, &headers_payload, header_block_fragment);
    frame_t frame;
    frame.frame_header = &frame_header;
    frame.payload = (void *)&headers_payload;

    uint8_t result_bytes[15];
    int size = frame_to_bytes(&frame, result_bytes);

    TEST_ASSERT_EQUAL(15, size);
    uint8_t expected_bytes[] = { 0, 0, 6, HEADERS_TYPE, 0, 0, 0, 0, 1,
                                 1, 2, 3, 4, 5, 6 };
    for (int i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], result_bytes[i]);
    }
}

void test_frame_to_bytes_continuation(void)
{
    uint8_t headers_block[6] = { 1, 2, 3, 4, 5, 6 };
    int headers_block_size = 6;
    uint32_t stream_id = 1;
    frame_header_t frame_header;
    continuation_payload_t continuation_payload;
    uint8_t header_block_fragment[6];


    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_6;
    uint32_31_to_byte_array_fake.custom_fake = uint32_31_to_byte_array_custom_fake_1;
    uint16_to_byte_array_fake.custom_fake = uint16_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;


    create_continuation_frame(headers_block, headers_block_size, stream_id, &frame_header, &continuation_payload, header_block_fragment);
    frame_t frame;
    frame.frame_header = &frame_header;
    frame.payload = (void *)&continuation_payload;

    uint8_t result_bytes[15];
    int size = frame_to_bytes(&frame, result_bytes);

    TEST_ASSERT_EQUAL(15, size);
    uint8_t expected_bytes[] = { 0, 0, 6, CONTINUATION_TYPE, 0, 0, 0, 0, 1,
                                 1, 2, 3, 4, 5, 6 };
    for (int i = 0; i < size; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], result_bytes[i]);
    }
}

int encode_fake_custom(hpack_states_t *hpack_states, char *name_string, char *value_string,  uint8_t *encoded_buffer)
{
    (void)hpack_states;
    (void)value_string;
    (void)name_string;


    //TEST_ASSERT_EQUAL_MESSAGE(0, index, "Index given to encode() should start at 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("hola", name_string, "Header name should be 'hola'");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("val", value_string, "Header value should be 'val'");

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;


    uint8_t expected_compressed_headers[] = {
        0,      //01000000 prefix=00, index=0
        4,      //h=0, name length = 4;
        'h',    //name string
        'o',    //name string
        'l',    //name string
        'a',    //name string
        3,      //h=0, value length = 3;
        'v',    //value string
        'a',    //value string
        'l'     //value string
    };
    buffer_copy(encoded_buffer, expected_compressed_headers, 10);
    return 10;
}

void test_compress_headers(void)
{
    headers_t headers;
    uint8_t compressed_headers[256];

    encode_fake.custom_fake = encode_fake_custom;

    // Set return values for headers functions
    headers_count_fake.return_val = 1;
    headers_get_name_from_index_fake.return_val = "hola";
    headers_get_value_from_index_fake.return_val = "val";

    int rc = compress_headers(&headers, compressed_headers, NULL);

    TEST_ASSERT_EQUAL(10, rc);
    uint8_t expected_compressed_headers[] = {
        0,      //00000000 prefix=00, index=0
        4,      //h=0, name length = 4;
        'h',    //name string
        'o',    //name string
        'l',    //name string
        'a',    //name string
        3,      //h=0, value length = 3;
        'v',    //value string
        'a',    //value string
        'l'     //value string
    };

    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_compressed_headers[i], compressed_headers[i]);
    }
}


void test_create_window_update_frame(void)
{
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    int window_size_increment = 30;
    uint32_t stream_id = 1;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;

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

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
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

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    bytes_to_uint32_31_fake.return_val = 30;
    int rc = read_window_update_payload(buff_read, &frame_header, &window_update_payload);

    TEST_ASSERT_EQUAL(4, rc);
    TEST_ASSERT_EQUAL(30, window_update_payload.window_size_increment);


}
void test_frame_to_bytes_data(void)
{
    frame_header_t frame_header;
    data_payload_t data_payload;
    int length = 10;
    uint32_t stream_id = 1;
    uint8_t data[10];
    uint8_t data_to_send[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 10 };

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_31_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_num;

    create_data_frame(&frame_header, &data_payload, data, data_to_send, length, stream_id);
    frame_t frame;
    frame.frame_header = &frame_header;
    frame.payload = (void *)&data_payload;
    uint8_t bytes[30];
    int rc = frame_to_bytes(&frame, bytes);

    uint8_t expected_bytes[] = {
        0, 0, 10,
        0x0,
        0x0,
        0, 0, 0, 1,
        1, 2, 3, 4, 5, 6, 7, 8, 9, 10
    };

    TEST_ASSERT_EQUAL(19, rc);
    for (int i = 0; i < rc; i++) {

        TEST_ASSERT_EQUAL(expected_bytes[i], bytes[i]);
    }

}

void test_frame_to_bytes_window_update(void)
{
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    uint32_t stream_id = 1;//1
    uint32_t window_size_increment = 30;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_31_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_num;

    int rc = create_window_update_frame(&frame_header, &window_update_payload, window_size_increment, stream_id);
    TEST_ASSERT_EQUAL(0, rc);
    frame_t frame;
    frame.frame_header = &frame_header;
    frame.payload = (void *)&window_update_payload;
    uint8_t bytes[30];
    rc = frame_to_bytes(&frame, bytes);

    uint8_t expected_bytes[] = {
        0, 0, 4,
        0x8,
        0x0,
        0, 0, 0, 1,
        0, 0, 0, 30
    };
    TEST_ASSERT_EQUAL(13, rc);
    for (int i = 0; i < rc; i++) {
        printf("i:%d", i);
        TEST_ASSERT_EQUAL(expected_bytes[i], bytes[i]);
    }
}

void test_frame_to_bytes_goaway(void)
{
    frame_header_t frame_header;
    goaway_payload_t goaway_payload;

    uint32_t last_stream_id = 30;
    uint32_t error_code = 1;
    uint8_t additional_debug_data_buffer[4];
    uint8_t additional_debug_data[] = { 1, 2, 3, 4 };
    uint8_t additional_debug_data_size = 4;

    buffer_copy_fake.custom_fake = buffer_copy_fake_custom;
    append_byte_arrays_fake.custom_fake = append_byte_arrays_custom_fake;
    uint32_31_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_to_byte_array_fake.custom_fake = uint32_to_byte_array_custom_fake_num;
    uint32_24_to_byte_array_fake.custom_fake = uint32_24_to_byte_array_custom_fake_num;



    int rc = create_goaway_frame(&frame_header, &goaway_payload, additional_debug_data_buffer, last_stream_id, error_code, additional_debug_data, additional_debug_data_size);
    TEST_ASSERT_EQUAL(0, rc);
    frame_t frame;
    frame.frame_header = &frame_header;
    frame.payload = (void *)&goaway_payload;
    uint8_t bytes[30];
    rc = frame_to_bytes(&frame, bytes);

    uint8_t expected_bytes[] = {
        0, 0, 12,
        0x7,
        0x0,
        0, 0, 0, 0,

        0, 0, 0, 30,
        0, 0, 0, 1,
        1, 2, 3, 4
    };

    TEST_ASSERT_EQUAL(12 + 9, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes[i], bytes[i]);
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
    UNIT_TEST(test_read_settings_payload);

    UNIT_TEST(test_create_settings_ack_frame);
    UNIT_TEST(test_frame_to_bytes_settings);
    UNIT_TEST(test_read_headers_payload);
    UNIT_TEST(test_create_headers_frame);

    UNIT_TEST(test_headers_payload_to_bytes);

    UNIT_TEST(test_frame_to_bytes_headers)

    UNIT_TEST(test_frame_to_bytes_continuation)


    UNIT_TEST(test_compress_headers);
    UNIT_TEST(test_frame_to_bytes_data);

    UNIT_TEST(test_create_window_update_frame);
    UNIT_TEST(test_window_update_payload_to_bytes);
    UNIT_TEST(test_read_window_update_payload);
    UNIT_TEST(test_frame_to_bytes_window_update);

    UNIT_TEST(test_frame_to_bytes_goaway);



    return UNIT_TESTS_END();
}
