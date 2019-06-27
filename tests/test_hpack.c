#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "logging.h"
#include "hpack.h"
#include "table.h"

extern int log128(uint32_t x);


DEFINE_FFF_GLOBALS;
/*FAKE_VALUE_FUNC(int, uint32_24_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t*);

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t*);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t*);

FAKE_VALUE_FUNC(int, append_byte_arrays, uint8_t*, uint8_t*, uint8_t*, int, int);
FAKE_VALUE_FUNC(int, buffer_copy, uint8_t*, uint8_t*, int);

FAKE_VALUE_FUNC(int, encode, hpack_preamble_t , uint32_t , uint32_t ,char*, uint8_t , char*, uint8_t , uint8_t*);
FAKE_VALUE_FUNC(int, decode_header_block, uint8_t* , uint8_t , headers_lists_t*);
*/

/* List of fakes used by this unit tester */
/*#define FFF_FAKES_LIST(FAKE)        \
    FAKE(uint32_24_to_byte_array)   \
    FAKE(uint32_31_to_byte_array)   \
    FAKE(uint32_to_byte_array)      \
    FAKE(uint16_to_byte_array)      \
    FAKE(bytes_to_uint32)           \
    FAKE(bytes_to_uint32_31)        \
    FAKE(bytes_to_uint32_24)        \
    FAKE(bytes_to_uint16)           \
    FAKE(append_byte_arrays)        \
    FAKE(buffer_copy)               \
    FAKE(encode)                    \
    FAKE(decode_header_block)
*/

void setUp(void) {
    /* Register resets */
    //FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_decode_header_block(void){
    //Literal Header Field Representation
        //without indexing
            //new name
    uint8_t header_block_size = 10;
    uint8_t header_block[]={
            0,//01000000 prefix=00, index=0
            4,//h=0, name length = 4;
            'h',//name string
            'o',//name string
            'l',//name string
            'a',//name string
            3, //h=0, value length = 3;
            'v',//value string
            'a',//value string
            'l'//value string
    };
    char expected_name[] ="hola";
    char expected_value[] ="val";

    headers_lists_t h_list;
    int rc = decode_header_block(header_block, header_block_size, &h_list);
    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    INFO("%s\n",h_list.header_list_in[0].name);
    INFO("%s\n",h_list.header_list_in[0].value);
    TEST_ASSERT_EQUAL(0,strcmp(expected_name,h_list.header_list_in[0].name));
    TEST_ASSERT_EQUAL(0,strcmp(expected_value,h_list.header_list_in[0].value));


}

void test_encode(void){
    hpack_preamble_t preamble = LITERAL_HEADER_FIELD_WITHOUT_INDEXING;
    uint32_t max_size = 0;
    uint32_t index = 0;
    char value_string[]="val";
    uint8_t value_huffman_bool = 0;
    char name_string[] = "name";
    uint8_t name_huffman_bool = 0;
    uint8_t encoded_buffer[64];

    uint8_t expected_encoded_bytes[] = {
            0,//LITERAL_HEADER_FIELD_WITHOUT_INDEXING, index=0
            4,//name_length
            (uint8_t)'n',
            (uint8_t)'a',
            (uint8_t)'m',
            (uint8_t)'e',
            3,//value_length
            (uint8_t)'v',
            (uint8_t)'a',
            (uint8_t)'l'
    };

    int rc = encode(preamble, max_size, index, value_string, value_huffman_bool, name_string, name_huffman_bool, encoded_buffer);


    TEST_ASSERT_EQUAL(10,rc);
    for(int i = 0; i< rc; i++){
        TEST_ASSERT_EQUAL(expected_encoded_bytes[i],encoded_buffer[i]);
    }
}


void test_log128(void) {
    TEST_ASSERT_EQUAL(0,log128(1));
    TEST_ASSERT_EQUAL(0,log128(2));
    TEST_ASSERT_EQUAL(0,log128(3));
    TEST_ASSERT_EQUAL(0,log128(4));
    TEST_ASSERT_EQUAL(0,log128(5));
    TEST_ASSERT_EQUAL(0,log128(25));
    TEST_ASSERT_EQUAL(0,log128(127));
    TEST_ASSERT_EQUAL(1,log128(128));
    TEST_ASSERT_EQUAL(1,log128(387));
    TEST_ASSERT_EQUAL(1,log128(4095));
    TEST_ASSERT_EQUAL(1,log128(16383));
    TEST_ASSERT_EQUAL(2,log128(16384));
    TEST_ASSERT_EQUAL(2,log128(1187752));
    TEST_ASSERT_EQUAL(3,log128(2097152));

}


int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_decode_header_block);
    UNIT_TEST(test_encode);
    UNIT_TEST(test_log128);
    /*UNIT_TEST(test_frame_header_to_bytes);
    UNIT_TEST(test_frame_header_to_bytes_reserved);
    UNIT_TEST(test_bytes_to_frame_header);

    UNIT_TEST(test_create_list_of_settings_pair);
    UNIT_TEST(test_create_settings_frame);
    UNIT_TEST(test_setting_to_bytes);
    UNIT_TEST(test_settings_frame_to_bytes);
    UNIT_TEST(test_bytes_to_settings_payload);

    UNIT_TEST(test_create_settings_ack_frame);
    UNIT_TEST(test_frame_to_bytes_settings);
    UNIT_TEST(test_read_headers_payload);
    UNIT_TEST(test_read_continuation_payload);
    UNIT_TEST(test_create_headers_frame);
    UNIT_TEST(test_create_continuation_frame);

    UNIT_TEST(test_headers_payload_to_bytes);

    UNIT_TEST(test_frame_to_bytes_headers)

    UNIT_TEST(test_frame_to_bytes_continuation)

    UNIT_TEST(test_frame_to_bytes_continuation)
    UNIT_TEST(test_continuation_payload_to_bytes);
    */


    return UNIT_TESTS_END();
}
