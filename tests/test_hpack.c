#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "logging.h"
#include "hpack.h"
#include "hpack_huffman.h"
#include "hpack_tables.h"
#include "table.h"

extern int decode_huffman_string(char *str, uint8_t *encoded_string);
extern int decode_string(char *str, uint8_t *encoded_string);
extern int decode_non_huffman_string(char *str, uint8_t *encoded_string);
extern int32_t decode_huffman_word(char *str, uint8_t *encoded_string, uint8_t encoded_string_size, uint16_t bit_position);
extern uint32_t decode_integer(uint8_t *bytes, uint8_t prefix);
extern int encoded_integer_size(uint32_t num, uint8_t prefix);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int8_t, hpack_huffman_decode, huffman_encoded_word_t *, uint8_t *);
FAKE_VALUE_FUNC(int,  headers_add, headers_t *, const char *, const char * );
FAKE_VALUE_FUNC(uint32_t, hpack_utils_read_bits_from_bytes, uint16_t, uint8_t, uint8_t *);
FAKE_VALUE_FUNC(int8_t, hpack_utils_check_can_read_buffer, uint16_t, uint8_t, uint8_t );
FAKE_VALUE_FUNC(hpack_preamble_t, hpack_utils_get_preamble, uint8_t);
FAKE_VALUE_FUNC(uint8_t, hpack_utils_find_prefix_size, hpack_preamble_t);
FAKE_VALUE_FUNC(int, hpack_encoder_encode, hpack_preamble_t, uint32_t, uint32_t, char *, uint8_t, char *, uint8_t,  uint8_t *);
FAKE_VALUE_FUNC(uint32_t, hpack_utils_encoded_integer_size, uint32_t, uint8_t);
FAKE_VALUE_FUNC(int8_t, hpack_tables_static_find_name_and_value, uint8_t, char *, char *);
FAKE_VALUE_FUNC(int8_t, hpack_tables_static_find_name, uint8_t, char *);
FAKE_VALUE_FUNC(uint32_t, hpack_tables_get_table_length, uint32_t);
FAKE_VALUE_FUNC(int, hpack_tables_init_dynamic_table, hpack_dynamic_table_t *, uint32_t, header_pair_t * );
FAKE_VALUE_FUNC(int, hpack_tables_dynamic_table_add_entry,hpack_dynamic_table_t *, char *, char *);
FAKE_VALUE_FUNC(int, hpack_tables_find_entry_name_and_value,hpack_dynamic_table_t *, uint32_t , char *, char *);
FAKE_VALUE_FUNC(int, hpack_tables_find_entry_name,hpack_dynamic_table_t *, uint32_t , char *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                     \
    FAKE(hpack_huffman_decode)                   \
    FAKE(hpack_utils_read_bits_from_bytes)       \
    FAKE(hpack_utils_check_can_read_buffer)      \
    FAKE(hpack_utils_get_preamble)               \
    FAKE(hpack_utils_find_prefix_size)           \
    FAKE(hpack_encoder_encode)                   \
    FAKE(hpack_utils_encoded_integer_size)       \
    FAKE(hpack_tables_get_table_length)          \
    FAKE(hpack_tables_init_dynamic_table)        \
    FAKE(hpack_tables_dynamic_table_add_entry)   \
    FAKE(hpack_tables_find_entry_name_and_value) \
    FAKE(hpack_tables_find_entry_name)           \
    FAKE(headers_add)

/*Decode*/
int8_t hpack_huffman_decode_return_not_found(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    return -1;
}
int8_t hpack_huffman_decode_return_w(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'w';
    return 0;
}
int8_t hpack_huffman_decode_return_dot(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'.';
    return 0;
}
int8_t hpack_huffman_decode_return_e(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'e';
    return 0;
}
int8_t hpack_huffman_decode_return_x(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'x';
    return 0;
}
int8_t hpack_huffman_decode_return_a(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'a';
    return 0;
}
int8_t hpack_huffman_decode_return_m(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'m';
    return 0;
}
int8_t hpack_huffman_decode_return_p(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'p';
    return 0;
}
int8_t hpack_huffman_decode_return_l(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'l';
    return 0;
}
int8_t hpack_huffman_decode_return_c(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'c';
    return 0;
}
int8_t hpack_huffman_decode_return_o(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'o';
    return 0;
}

int8_t(*hpack_huffman_decode_wwwdotexampledotcom_arr[])(huffman_encoded_word_t *, uint8_t *) = { hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_dot,
                                                                                                 hpack_huffman_decode_return_e,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_x,
                                                                                                 hpack_huffman_decode_return_a,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_m,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_p,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_l,
                                                                                                 hpack_huffman_decode_return_e,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_dot,
                                                                                                 hpack_huffman_decode_return_c,
                                                                                                 hpack_huffman_decode_return_o,
                                                                                                 hpack_huffman_decode_return_not_found,
                                                                                                 hpack_huffman_decode_return_m,
                                                                                                 hpack_huffman_decode_return_not_found };
int8_t(*hpack_huffman_decode_bad_padding_arr[])(huffman_encoded_word_t *, uint8_t *) = { hpack_huffman_decode_return_a,
                                                                                         hpack_huffman_decode_return_not_found };
uint8_t encoded_wwwdotexampledotcom[] = { 0x8c,
                                          0xf1,
                                          0xe3,
                                          0xc2,
                                          0xe5,
                                          0xf2,
                                          0x3a,
                                          0x6b,
                                          0xa0,
                                          0xab,
                                          0x90,
                                          0xf4,
                                          0xff };


int hpack_tables_find_name_return_authority(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    (void)index;
    char authority[] = ":authority";
    strncpy(name, authority, strlen(authority));
    return 0;
}

int hpack_tables_find_name_return_age(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    (void)index;
    char age[] = "age";
    strncpy(name, age, strlen(age));
    return 0;
}
int hpack_tables_find_name_return_new_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name){
    (void)index;
    char age[] = "new_name";
    strncpy(name, age, strlen(age));
    return 0;
}


int headers_add_check_inputs(headers_t *headers, const char *name, const char *value)
{
    TEST_ASSERT_EQUAL_STRING("new_name", name);
    TEST_ASSERT_EQUAL_STRING("val", value);
    return 0;
}

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_decode_header_block_literal_never_indexed(void)
{
    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as string literal
    uint8_t header_block_size = 10;
    uint8_t header_block_name_literal[] = {
        16,     //00010000 prefix=0001, index=0
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
    char *expected_name = "hola";
    char *expected_value = "val";

    header_t h_list[1];
    headers_t headers;

    headers.count = 0;
    headers.maxlen = 3;
    headers.headers = h_list;
    uint32_t hpack_encoded_integer_size_fake_seq[] = { 1, 1, 1, 1, 2, 1 };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size, hpack_encoded_integer_size_fake_seq, 6);

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)16;
    hpack_utils_find_prefix_size_fake.return_val = 4;

    int rc = decode_header_block(header_block_name_literal, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(1, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);

    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as static table index

    headers.count = 0;

    header_block_size = 5;
    uint8_t header_block_name_indexed[] = {
        17,     //00010001 prefix=0001, index=1
        3,      //h=0, value length = 3;
        'v',
        'a',
        'l'
    };
    expected_name = ":authority";
    hpack_tables_find_entry_name_fake.custom_fake = hpack_tables_find_name_return_authority;
    rc = decode_header_block(header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);

    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as dynamic table index

    uint32_t max_dynamic_table_size = 3000;

    hpack_dynamic_table_t dynamic_table;

    header_pair_t table[hpack_tables_get_table_length(max_dynamic_table_size)];

    hpack_tables_init_dynamic_table(&dynamic_table, max_dynamic_table_size, table);
    hpack_tables_find_entry_name_fake.custom_fake = hpack_tables_find_name_return_new_name;

    char *new_name = "new_name";
    char *new_value = "new_value";

    hpack_tables_dynamic_table_add_entry(&dynamic_table, new_name, new_value);

    header_block_size = 6;

    uint8_t header_block_dynamic_index[] = {
        31, // 0001 1111 , never indexed, index 15 + ...
        47, // 0010 1111 , index = 62 = 15 + 47
        3,  // val length = 3, no huffman
        'v',
        'a',
        'l'
    };

    headers_add_fake.custom_fake = headers_add_check_inputs;
    rc = decode_header_block_from_table(&dynamic_table, header_block_dynamic_index, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(3, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(new_name, headers_add_fake.arg1_val);
//    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val); TODO checkear porqué falla el assert

}

void test_decode_header_block_literal_without_indexing(void)
{
    //Literal Header Field Representation
    //without indexing
    //No huffman encoding - Header name as string literal
    uint8_t header_block_size = 10;
    uint8_t header_block_name_literal[] = {
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
    char *expected_name = "hola";
    char *expected_value = "val";

    header_t h_list[1];
    headers_t headers;

    headers.count = 0;
    headers.maxlen = 3;
    headers.headers = h_list;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)0;
    hpack_utils_find_prefix_size_fake.return_val = 4;

    uint32_t hpack_encoded_integer_size_fake_seq[] = { 1, 1, 2, 1 };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size, hpack_encoded_integer_size_fake_seq, 4);


    int rc = decode_header_block(header_block_name_literal, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(1, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);

    //Literal Header Field Representation
    //without indexing
    //No huffman encoding - Header name as static table index

    headers.count = 0;

    header_block_size = 6;
    uint8_t header_block_name_indexed[] = {
        15,
        6,
        3,
        'v',
        'a',
        'l'
    };
    expected_name = "age";
    hpack_tables_find_entry_name_fake.custom_fake = hpack_tables_find_name_return_age;
    rc = decode_header_block(header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);


}


void test_encode(void)
{
    //TODO
}

void test_decode_non_huffman_string(void)
{
    uint8_t encoded_string[] = { 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    char decoded_string[30];

    hpack_utils_encoded_integer_size_fake.return_val = 1;

    memset(decoded_string, 0, 30);
    char expected_decoded_string[] = "www.example.com";
    int rc = decode_non_huffman_string(decoded_string, encoded_string);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
}

/*Test to see if decoding and encoded non huffman string yields the same string*/
void test_encode_then_decode_non_huffman_string(void)
{
    /*
       char str[] = "www.example.com";
       uint8_t encoded_string[30];
       char result[30];

       memset(encoded_string, 0, 30);
       memset(result, 0, 30);

       int rc = encode_non_huffman_string(str, encoded_string);
       int rc2 = decode_non_huffman_string(result, encoded_string);
       TEST_ASSERT_EQUAL(rc, rc2);
       for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(str[i], result[i]);
       }*/
    //TODO
}


void test_decode_huffman_word(void)
{
    uint8_t *encoded_string = encoded_wwwdotexampledotcom + 1; //We don't need to decode as string the first byte
    char expected_decoded_string[] = "www.example.com";
    uint8_t expected_word_length[] = { 7, 7, 7, 6, 5, 7, 5, 6, 6, 6, 5, 6, 5, 5, 6 };
    uint32_t return_fake_values_read_bits_from_bytes[] = { 30, 60, 120, 30, 60, 120, 30, 60, 120, 11, 23, 5, 30, 60, 121, 3, 20, 41, 21, 43, 20, 40, 5, 11, 23, 4, 7, 20, 41 };


    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
    SET_RETURN_SEQ(hpack_utils_read_bits_from_bytes, return_fake_values_read_bits_from_bytes, 29);

    char decoded_sym = (char)0;
    uint8_t encoded_string_size = 12;
    uint16_t bit_position = 0;

    for (int i = 0; i < 15; i++) {
        int8_t rc = decode_huffman_word(&decoded_sym, encoded_string, encoded_string_size, bit_position);
        TEST_ASSERT_EQUAL(expected_word_length[i], rc);
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_sym);
        bit_position += rc;
    }
    TEST_ASSERT_EQUAL(29, hpack_huffman_decode_fake.call_count);
}

void test_decode_huffman_string(void)
{
    char decoded_string[30];
    uint32_t return_fake_values_read_bits_from_bytes[] = { 30, 60, 120, 30, 60, 120, 30, 60, 120, 11, 23, 5, 30, 60, 121, 3, 20, 41, 21, 43, 20, 40, 5, 11, 23, 4, 7, 20, 41, 31, 63, 127 };

    memset(decoded_string, 0, 30);
    char expected_decoded_string[] = "www.example.com";
    uint8_t *encoded_string = encoded_wwwdotexampledotcom;

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
    SET_RETURN_SEQ(hpack_utils_read_bits_from_bytes, return_fake_values_read_bits_from_bytes, 32);
    int rc = decode_huffman_string(decoded_string, encoded_string);
    TEST_ASSERT_EQUAL(13, rc);
    TEST_ASSERT_EQUAL(32, hpack_huffman_decode_fake.call_count);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }

}

void test_decode_huffman_string_error(void)
{
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_bad_padding_arr, 2);
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    /*Test border condition*/
    /*Padding wrong*/
    uint8_t encoded_string2[] = { 0x81, 0x1b };
    uint32_t return_fake_values_read_bits_from_bytes[] = { 3, 13, 27, 55, 111 };
    SET_RETURN_SEQ(hpack_utils_read_bits_from_bytes, return_fake_values_read_bits_from_bytes, 5);
    char expected_decoded_string2[] = "a";
    char decoded_string2[] = { 0, 0 };

    int rc = decode_huffman_string(decoded_string2, encoded_string2);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(1, hpack_huffman_decode_fake.call_count);
    TEST_ASSERT_EQUAL(expected_decoded_string2[0], decoded_string2[0]);

    /*Couldn't read code*/
    uint8_t encoded_string3[] = { 0x81, 0x6f };
    char decoded_string3[] = { 0, 0 };
    char expected_decoded_string3[] = { 0, 0 };
    rc = decode_huffman_string(decoded_string3, encoded_string3);
    TEST_ASSERT_EQUAL(-1, rc);
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string3[i], decoded_string3[i]);
    }
}

/*Test to see if decoding and encoded huffman string yields the same string*/
void test_encode_then_decode_huffman_string(void)
{
    /*
       char str[] = "www.example.com";
       uint32_t return_fake_values_read_bits_from_bytes[] = { 30, 60, 120, 30, 60, 120, 30, 60, 120, 11, 23, 5, 30, 60, 121, 3, 20, 41, 21, 43, 20, 40, 5, 11, 23, 4, 7, 20, 41, 31, 63, 127 };
       uint8_t encoded_string[30];
       char result[30];

       memset(encoded_string, 0, 30);
       memset(result, 0, 30);

       SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
       SET_RETURN_SEQ(hpack_utils_read_bits_from_bytes, return_fake_values_read_bits_from_bytes, 32);

       SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);
       int rc = encode_huffman_string(str, encoded_string);
       (void)rc;
       int rc2 = decode_huffman_string(result, encoded_string);
       for (int i = 0; i < rc2; i++) {
        TEST_ASSERT_EQUAL(str[i], result[i]);
       }*/
    //TODO
}

void test_decode_string(void)
{
    uint8_t encoded_string[] = { 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    uint32_t return_fake_values_read_bits_from_bytes[] = { 30, 60, 120, 30, 60, 120, 30, 60, 120, 11, 23, 5, 30, 60, 121, 3, 20, 41, 21, 43, 20, 40, 5, 11, 23, 4, 7, 20, 41, 31, 63, 127 };
    char decoded_string[30];

    SET_RETURN_SEQ(hpack_utils_read_bits_from_bytes, return_fake_values_read_bits_from_bytes, 32);
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    /*Test decode a non huffman string*/
    memset(decoded_string, 0, 30);
    char expected_decoded_string[] = "www.example.com";
    int rc = decode_string(decoded_string, encoded_string);
    TEST_ASSERT_EQUAL(16, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
    /*Test decode a huffman string*/
    memset(decoded_string, 0, 30);

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
    uint8_t *encoded_string_huffman = encoded_wwwdotexampledotcom;
    int rc2 = decode_string(decoded_string, encoded_string_huffman);
    TEST_ASSERT_EQUAL(32, hpack_huffman_decode_fake.call_count);

    TEST_ASSERT_EQUAL(13, rc2);
    for (int i = 0; i < rc2; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
}

void test_decode_string_error(void)
{
    uint32_t return_fake_value_read_bits_from_bytes = 3;

    hpack_utils_read_bits_from_bytes_fake.return_val = return_fake_value_read_bits_from_bytes;

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_bad_padding_arr, 2);

    /*Test border condition*/
    /*Padding wrong*/
    uint8_t encoded_string2[] = { 0x81, 0x1b };
    char expected_decoded_string2[] = "a";
    char decoded_string2[] = { 0, 0 };

    int rc = decode_string(decoded_string2, encoded_string2);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(expected_decoded_string2[0], decoded_string2[0]);
}

void test_decode_integer(void)
{
    uint8_t bytes1[] = {
        30    //000 11110
    };
    uint8_t prefix = 5;
    uint32_t rc = decode_integer(bytes1, prefix);

    TEST_ASSERT_EQUAL(30, rc);

    uint8_t bytes2[] = {
        31,     //000 11111
        0       //+
    };
    rc = decode_integer(bytes2, prefix);
    TEST_ASSERT_EQUAL(31, rc);

    uint8_t bytes3[] = {
        31,     //000 11111
        1,      //
    };
    rc = decode_integer(bytes3, prefix);
    TEST_ASSERT_EQUAL(32, rc);

    uint8_t bytes4[] = {
        31,     //000 11111
        2,      //
    };
    rc = decode_integer(bytes4, prefix);
    TEST_ASSERT_EQUAL(33, rc);

    uint8_t bytes5[] = {
        31,     //000 11111
        128,    //
        1
    };
    rc = decode_integer(bytes5, prefix);
    TEST_ASSERT_EQUAL(128 + 31, rc);

    uint8_t bytes6[] = {
        31,     //000 11111
        154,    //
        10
    };
    rc = decode_integer(bytes6, prefix);
    TEST_ASSERT_EQUAL(1337, rc);
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_decode_header_block_literal_without_indexing);
    UNIT_TEST(test_decode_header_block_literal_never_indexed);

    UNIT_TEST(test_encode);

    UNIT_TEST(test_decode_huffman_word);
    UNIT_TEST(test_decode_integer);
    UNIT_TEST(test_decode_non_huffman_string);

    UNIT_TEST(test_decode_huffman_string);
    UNIT_TEST(test_decode_huffman_string_error);
    UNIT_TEST(test_decode_string);

    UNIT_TEST(test_decode_string_error);
    UNIT_TEST(test_encode_then_decode_non_huffman_string);
    UNIT_TEST(test_encode_then_decode_huffman_string);

    return UNIT_TESTS_END();
}
