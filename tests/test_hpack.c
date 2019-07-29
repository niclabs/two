#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "logging.h"
#include "hpack.h"
#include "hpack_huffman.h"
#include "table.h"


void tearDown(void);

extern int log128(uint32_t x);
extern hpack_preamble_t get_preamble(uint8_t preamble);
extern int8_t read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer, uint8_t buffer_size, uint32_t *result);
extern int8_t pack_encoded_words_to_bytes(huffman_encoded_word_t *encoded_words, uint8_t encoded_words_size, uint8_t *buffer, uint8_t buffer_size);
extern int encoded_integer_size(uint32_t num, uint8_t prefix);
extern int encode_huffman_string(char *str, uint8_t *encoded_string);
extern int decode_huffman_string(char *str, uint8_t *encoded_string);
extern int decode_string(char *str, uint8_t *encoded_string);
extern int encode_non_huffman_string(char *str, uint8_t *encoded_string);
extern int decode_non_huffman_string(char *str, uint8_t *encoded_string);
extern uint32_t encode_huffman_word(char *str, int str_length, huffman_encoded_word_t *encoded_words);
extern int32_t decode_huffman_word(char *str, uint8_t *encoded_string, uint8_t encoded_string_size, uint16_t bit_position);
extern uint8_t find_prefix_size(hpack_preamble_t octet);
extern uint32_t decode_integer(uint8_t *bytes, uint8_t prefix);
extern int encode_literal_header_field_new_name( char *name_string, uint8_t name_huffman_bool, char *value_string, uint8_t value_huffman_bool, uint8_t *encoded_buffer);
extern int encode_integer(uint32_t integer, uint8_t prefix, uint8_t *encoded_integer);
extern int dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);


DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int8_t, hpack_huffman_encode, huffman_encoded_word_t *, uint8_t);
FAKE_VALUE_FUNC(int8_t, hpack_huffman_decode, huffman_encoded_word_t *, uint8_t *);
FAKE_VALUE_FUNC(int,  headers_add, headers_t * , const char * , const char * );


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
   FAKE_VALUE_FUNC(int, decode_header_block, uint8_t* , uint8_t , headers_data_lists_t*);
 */

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)        \
    FAKE(hpack_huffman_encode)      \
    FAKE(hpack_huffman_decode)      \
    FAKE(headers_add)
/*    FAKE(uint32_24_to_byte_array)   \
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

/*----------Value Return for FAKEs ----------*/
int8_t hpack_huffman_encode_return_w(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x78;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_dot(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x17;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_a(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x3;
    h->length = 5;
    return 0;
}

int8_t hpack_huffman_encode_return_e(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x5;
    h->length = 5;
    return 0;
}

int8_t hpack_huffman_encode_return_x(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x79;
    h->length = 7;
    return 0;
}

int8_t hpack_huffman_encode_return_m(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x29;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_p(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x2b;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_l(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x28;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_c(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x4;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_o(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x7;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_u(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x2d;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_s(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x8;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_t(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x9;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_minus(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x16;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_k(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x75;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_y(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x7a;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_v(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code = 0x77;
    h->length = 7;
    return 0;
}
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
int8_t(*hpack_huffman_encode_wwwdotexampledotcom_arr[])(huffman_encoded_word_t *, uint8_t) = { hpack_huffman_encode_return_w,
                                                                                               hpack_huffman_encode_return_w,
                                                                                               hpack_huffman_encode_return_w,
                                                                                               hpack_huffman_encode_return_dot,
                                                                                               hpack_huffman_encode_return_e,
                                                                                               hpack_huffman_encode_return_x,
                                                                                               hpack_huffman_encode_return_a,
                                                                                               hpack_huffman_encode_return_m,
                                                                                               hpack_huffman_encode_return_p,
                                                                                               hpack_huffman_encode_return_l,
                                                                                               hpack_huffman_encode_return_e,
                                                                                               hpack_huffman_encode_return_dot,
                                                                                               hpack_huffman_encode_return_c,
                                                                                               hpack_huffman_encode_return_o,
                                                                                               hpack_huffman_encode_return_m };

int8_t(*hpack_huffman_encode_customkey_arr[])(huffman_encoded_word_t *, uint8_t) = { hpack_huffman_encode_return_c,
                                                                                     hpack_huffman_encode_return_u,
                                                                                     hpack_huffman_encode_return_s,
                                                                                     hpack_huffman_encode_return_t,
                                                                                     hpack_huffman_encode_return_o,
                                                                                     hpack_huffman_encode_return_m,
                                                                                     hpack_huffman_encode_return_minus,
                                                                                     hpack_huffman_encode_return_k,
                                                                                     hpack_huffman_encode_return_e,
                                                                                     hpack_huffman_encode_return_y };


int8_t(*hpack_huffman_encode_customvalue_arr[])(huffman_encoded_word_t *, uint8_t) = { hpack_huffman_encode_return_c,
                                                                                       hpack_huffman_encode_return_u,
                                                                                       hpack_huffman_encode_return_s,
                                                                                       hpack_huffman_encode_return_t,
                                                                                       hpack_huffman_encode_return_o,
                                                                                       hpack_huffman_encode_return_m,
                                                                                       hpack_huffman_encode_return_minus,
                                                                                       hpack_huffman_encode_return_v,
                                                                                       hpack_huffman_encode_return_a,
                                                                                       hpack_huffman_encode_return_l,
                                                                                       hpack_huffman_encode_return_u,
                                                                                       hpack_huffman_encode_return_e };

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
void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_get_preamble(void)
{
    uint8_t preamble_arr[] = { (uint8_t)INDEXED_HEADER_FIELD,
                               (uint8_t)LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING,
                               (uint8_t)DYNAMIC_TABLE_SIZE_UPDATE,
                               (uint8_t)LITERAL_HEADER_FIELD_NEVER_INDEXED,
                               (uint8_t)LITERAL_HEADER_FIELD_WITHOUT_INDEXING };

    for (int i = 0; i < 5; i++) {
        TEST_ASSERT_EQUAL((hpack_preamble_t)preamble_arr[i], get_preamble(preamble_arr[i]));
    }

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
    rc = decode_header_block(header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);

    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as dynamic table index

    hpack_dynamic_table_t dynamic_table;

    hpack_init_dynamic_table(&dynamic_table, 20000);

    char *new_name = "new_name";
    char *new_value = "new_value";

    dynamic_table_add_entry(&dynamic_table, new_name, new_value);

    header_block_size = 6;

    uint8_t header_block_dynamic_index[] = {
        31, // 0001 1111 , never indexed, index 15 + ...
        47, // 0010 1111 , index = 62 = 15 + 47
        3, // val length = 3, no huffman
        'v',
        'a',
        'l'
    };

    rc = decode_header_block_from_table(&dynamic_table, header_block_dynamic_index, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(3, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(new_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);

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
    rc = decode_header_block(header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, headers_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, headers_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, headers_add_fake.arg2_val);


}


void test_encode(void)
{
    hpack_preamble_t preamble = LITERAL_HEADER_FIELD_WITHOUT_INDEXING;
    uint32_t max_size = 0;
    uint32_t index = 0;
    char value_string[] = "val";
    uint8_t value_huffman_bool = 0;
    char name_string[] = "name";
    uint8_t name_huffman_bool = 0;
    uint8_t encoded_buffer[64];

    uint8_t expected_encoded_bytes[] = {
        0,      //LITERAL_HEADER_FIELD_WITHOUT_INDEXING, index=0
        4,      //name_length
        (uint8_t)'n',
        (uint8_t)'a',
        (uint8_t)'m',
        (uint8_t)'e',
        3,    //value_length
        (uint8_t)'v',
        (uint8_t)'a',
        (uint8_t)'l'
    };

    int rc = encode(preamble, max_size, index, name_string, name_huffman_bool, value_string, value_huffman_bool, encoded_buffer);


    TEST_ASSERT_EQUAL(10, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_bytes[i], encoded_buffer[i]);
    }
}

void test_encoded_integer_size(void)
{
    uint32_t integer = 10;
    uint8_t prefix = 5;

    INFO("encoded: %u with prefix %u", integer, prefix);
    int rc = encoded_integer_size(integer, prefix);
    TEST_ASSERT_EQUAL(1, rc);

    integer = 30;
    prefix = 5;
    INFO("encoded: %u with prefix %u", integer, prefix);
    rc = encoded_integer_size(integer, prefix);
    TEST_ASSERT_EQUAL(1, rc);

    integer = 31;
    prefix = 5;
    INFO("encoded: %u with prefix %u", integer, prefix);
    rc = encoded_integer_size(integer, prefix);
    TEST_ASSERT_EQUAL(2, rc);

    integer = 31;
    prefix = 6;
    INFO("encoded: %u with prefix %u", integer, prefix);
    rc = encoded_integer_size(integer, prefix);
    TEST_ASSERT_EQUAL(1, rc);

    integer = 1337;
    prefix = 5;
    INFO("encoded: %u with prefix %u", integer, prefix);
    rc = encoded_integer_size(integer, prefix);
    TEST_ASSERT_EQUAL(3, rc);
}

void test_encode_integer(void)
{

    uint32_t integer = 10;
    uint8_t prefix = 5;
    int rc = encoded_integer_size(integer, prefix);
    uint8_t encoded_integer[4];

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes1[] = {
        10    //0,0,0, 0,1,0,1,0
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes1[i], encoded_integer[i]);
    }

    integer = 30;
    prefix = 5;
    rc = encoded_integer_size(integer, prefix);

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes2[] = {
        30    //0,0,0, 1,1,1,1,0
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes2[i], encoded_integer[i]);
    }

    integer = 128 + 31;
    prefix = 5;
    rc = encoded_integer_size(integer, prefix);

    for (int i = 0; i < 5; i++) {
        encoded_integer[i] = 0;
    }

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(3, rc);
    uint8_t expected_bytes6[] = {
        31,    //0,0,0, 1,1,1,1,1
        128,
        1
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes6[i], encoded_integer[i]);
    }


    integer = 31;
    prefix = 5;
    rc = encoded_integer_size(integer, prefix);

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(2, rc);
    uint8_t expected_bytes3[] = {
        31,     //0,0,0, 1,1,1,1,1
        0       //0 000 0000
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes3[i], encoded_integer[i]);
    }


    integer = 31;
    prefix = 6;
    rc = encoded_integer_size(integer, prefix);

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes4[] = {
        31    //0,0, 0,1,1,1,1,1
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes4[i], encoded_integer[i]);
    }


    integer = 1337;
    prefix = 5;
    rc = encoded_integer_size(integer, prefix);

    rc = encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(3, rc);
    uint8_t expected_bytes5[] = {
        31,    //0,0,0, 1,1,1,1,1
        154,
        10
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes5[i], encoded_integer[i]);
    }
}

void test_encode_non_huffman_string(void)
{
    char str[] = "char_to_encode";//14
    uint8_t encoded_string[30];
    uint8_t expected_encoded_string[] = {
        14,
        'c',
        'h',
        'a',
        'r',
        '_',
        't',
        'o',
        '_',
        'e',
        'n',
        'c',
        'o',
        'd',
        'e'
    };
    int rc = encode_non_huffman_string(str, encoded_string);

    TEST_ASSERT_EQUAL(15, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_string[i], encoded_string[i]);
    }
}

void test_decode_non_huffman_string(void)
{
    uint8_t encoded_string[] = { 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    char decoded_string[30];

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
    }
}

void test_find_prefix_size(void)
{
    hpack_preamble_t octet = LITERAL_HEADER_FIELD_WITHOUT_INDEXING;//0000 0000

    uint8_t rc = find_prefix_size(octet);

    TEST_ASSERT_EQUAL(4, rc);

    octet = INDEXED_HEADER_FIELD;//1 0000000
    rc = find_prefix_size(octet);
    TEST_ASSERT_EQUAL(7, rc);

    octet = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;//01 000000
    rc = find_prefix_size(octet);
    TEST_ASSERT_EQUAL(6, rc);

    octet = DYNAMIC_TABLE_SIZE_UPDATE;//001 00000
    rc = find_prefix_size(octet);
    TEST_ASSERT_EQUAL(5, rc);

}

void test_log128(void)
{
    TEST_ASSERT_EQUAL(0, log128(1));
    TEST_ASSERT_EQUAL(0, log128(2));
    TEST_ASSERT_EQUAL(0, log128(3));
    TEST_ASSERT_EQUAL(0, log128(4));
    TEST_ASSERT_EQUAL(0, log128(5));
    TEST_ASSERT_EQUAL(0, log128(25));
    TEST_ASSERT_EQUAL(0, log128(127));
    TEST_ASSERT_EQUAL(1, log128(128));
    TEST_ASSERT_EQUAL(1, log128(387));
    TEST_ASSERT_EQUAL(1, log128(4095));
    TEST_ASSERT_EQUAL(1, log128(16383));
    TEST_ASSERT_EQUAL(2, log128(16384));
    TEST_ASSERT_EQUAL(2, log128(1187752));
    TEST_ASSERT_EQUAL(3, log128(2097152));

}

void test_pack_encoded_words_to_bytes_test1(void)
{
    huffman_encoded_word_t encoded_buffer[] = {/*www.example.com*/
        { .code = 0x78, .length = 7 },
        { .code = 0x78, .length = 7 },
        { .code = 0x78, .length = 7 },
        { .code = 0x17, .length = 6 },
        { .code = 0x5, .length = 5 },
        { .code = 0x79, .length = 7 },
        { .code = 0x3, .length = 5 },
        { .code = 0x29, .length = 6 },
        { .code = 0x2b, .length = 6 },
        { .code = 0x28, .length = 6 },
        { .code = 0x5, .length = 5 },
        { .code = 0x17, .length = 6 },
        { .code = 0x4, .length = 5 },
        { .code = 0x7, .length = 5 },
        { .code = 0x29, .length = 6 }
    };
    uint8_t buffer[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t expected_result[] = { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a, 0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff };

    int8_t rs = pack_encoded_words_to_bytes(encoded_buffer, 15, buffer, 12);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 12; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}


void test_pack_encoded_words_to_bytes_test2(void)
{
    huffman_encoded_word_t encoded_buffer[] = {/*no-cache*/
        { .code = 0x2a, .length = 6 },
        { .code = 0x7, .length = 5 },
        { .code = 0x16, .length = 6 },
        { .code = 0x4, .length = 5 },
        { .code = 0x3, .length = 5 },
        { .code = 0x4, .length = 5 },
        { .code = 0x27, .length = 6 },
        { .code = 0x5, .length = 5 },
    };
    uint8_t buffer[6] = { 0, 0, 0, 0, 0, 0 };
    //a8eb 1064 9cbf
    uint8_t expected_result[] = { 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf };

    int8_t rs = pack_encoded_words_to_bytes(encoded_buffer, 8, buffer, 6);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}

void test_pack_encoded_words_to_bytes_test3(void)
{
    huffman_encoded_word_t encoded_buffer[] = {/*custom-value*/
        { .code = 0x4, .length = 5 },
        { .code = 0x2d, .length = 6 },
        { .code = 0x8, .length = 5 },
        { .code = 0x9, .length = 5 },
        { .code = 0x7, .length = 5 },
        { .code = 0x29, .length = 6 },
        { .code = 0x16, .length = 6 },    /*-*/
        { .code = 0x77, .length = 7 },
        { .code = 0x3, .length = 5 },
        { .code = 0x28, .length = 6 },
        { .code = 0x2d, .length = 6 },
        { .code = 0x5, .length = 5 },
    };
    uint8_t buffer[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    //25a8 49e9 5bb8 e8b4 bf
    uint8_t expected_result[] = { 0x25, 0xa8, 0x49, 0xe9, 0x5b, 0xb8, 0xe8, 0xb4, 0xbf };

    int8_t rs = pack_encoded_words_to_bytes(encoded_buffer, 12, buffer, 9);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 9; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}

void test_read_bits_from_bytes(void)
{
    uint8_t buffer[] = { 0xD1, 0xC5, 0x6E };
    uint32_t code = 0;
    int8_t rs = -1;

    //Test if it reads 1 byte correctly
    rs = read_bits_from_bytes(0, 8, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0xD1, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(8, 8, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0xC5, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(16, 8, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x6E, code);
    TEST_ASSERT_EQUAL(0, rs);

    //Test reading different number of bits
    rs = read_bits_from_bytes(0, 1, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x1, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(0, 3, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x6, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(0, 5, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x1A, code);
    TEST_ASSERT_EQUAL(0, rs);

    //Test reading between bytes
    rs = read_bits_from_bytes(4, 8, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x1C, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(12, 8, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x56, code);
    TEST_ASSERT_EQUAL(0, rs);

    rs = read_bits_from_bytes(4, 16, buffer, 3, &code);
    TEST_ASSERT_EQUAL(0x1C56, code);
    TEST_ASSERT_EQUAL(0, rs);

    //Test border condition
    rs = read_bits_from_bytes(0, 25, buffer, 3, &code);
    TEST_ASSERT_EQUAL(-1, rs);

    rs = read_bits_from_bytes(8, 17, buffer, 3, &code);
    TEST_ASSERT_EQUAL(-1, rs);

    rs = read_bits_from_bytes(16, 10, buffer, 3, &code);
    TEST_ASSERT_EQUAL(-1, rs);

}

void test_encode_huffman_word(void)
{
    uint32_t str_length = 15;
    char *str = "www.example.com";

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);

    huffman_encoded_word_t encoded_words[str_length];
    uint32_t encoded_word_bit_length = encode_huffman_word(str, str_length, encoded_words);
    TEST_ASSERT_EQUAL(89, encoded_word_bit_length);

}

void test_encode_huffman_string(void)
{
    char *str = "www.example.com";
    uint8_t encoded_string[30];

    uint8_t *expected_encoded_string = encoded_wwwdotexampledotcom;

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);
    int rc = encode_huffman_string(str, encoded_string);
    TEST_ASSERT_EQUAL(13, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_string[i], encoded_string[i]);
    }



}

void test_decode_huffman_word(void)
{
    uint8_t *encoded_string = encoded_wwwdotexampledotcom + 1; //We don't need to decode as string the first byte
    char expected_decoded_string[] = "www.example.com";

    uint8_t expected_word_length[] = { 7, 7, 7, 6, 5, 7, 5, 6, 6, 6, 5, 6, 5, 5, 6 };


    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);

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

    memset(decoded_string, 0, 30);
    char expected_decoded_string[] = "www.example.com";
    uint8_t *encoded_string = encoded_wwwdotexampledotcom;

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);

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
    /*Test border condition*/
    /*Padding wrong*/
    uint8_t encoded_string2[] = { 0x81, 0x1b };
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
    char str[] = "www.example.com";
    uint8_t encoded_string[30];
    char result[30];

    memset(encoded_string, 0, 30);
    memset(result, 0, 30);

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);
    int rc = encode_huffman_string(str, encoded_string);
    (void)rc;
    int rc2 = decode_huffman_string(result, encoded_string);
    for (int i = 0; i < rc2; i++) {
        TEST_ASSERT_EQUAL(str[i], result[i]);
    }
}

void test_decode_string(void)
{
    uint8_t encoded_string[] = { 0x0f, 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    char decoded_string[30];

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

void test_encode_literal_header_field_new_name(void)
{
    /*Test encoding a non huffman string*/
    uint8_t expected_non_huffman_string_encoded [] = { 0x0a,
                                                       0x63,
                                                       0x75,
                                                       0x73,
                                                       0x74,
                                                       0x6f,
                                                       0x6d,
                                                       0x2d,
                                                       0x6b,
                                                       0x65,
                                                       0x79,
                                                       0x0c,
                                                       0x63,
                                                       0x75,
                                                       0x73,
                                                       0x74,
                                                       0x6f,
                                                       0x6d,
                                                       0x2d,
                                                       0x76,
                                                       0x61,
                                                       0x6c,
                                                       0x75,
                                                       0x65 };

    char *name_to_encode = "custom-key";
    char *value_to_encode = "custom-value";
    uint8_t encoded_buffer[24];

    memset(encoded_buffer, 0, 24);
    uint8_t name_huffman_bool = 0;
    uint8_t value_huffman_bool = 0;
    int rc = encode_literal_header_field_new_name(name_to_encode,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_buffer);
    TEST_ASSERT_EQUAL(24, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_non_huffman_string_encoded[i], encoded_buffer[i]);
    }
    /*Test encoding a huffman string*/
    uint8_t expected_huffman_string_encoded [] = { 0x88,
                                                   0x25,
                                                   0xa8,
                                                   0x49,
                                                   0xe9,
                                                   0x5b,
                                                   0xa9,
                                                   0x7d,
                                                   0x7f,
                                                   0x89,
                                                   0x25,
                                                   0xa8,
                                                   0x49,
                                                   0xe9,
                                                   0x5b,
                                                   0xb8,
                                                   0xe8,
                                                   0xb4,
                                                   0xbf };
    uint8_t encoded_buffer_huffman[19];
    memset(encoded_buffer_huffman, 0, 19);

    /*Set up encoding function call*/
    int8_t(*hpack_huffman_encode_huffman_header_name_value[22])(huffman_encoded_word_t *, uint8_t);
    for (uint8_t i = 0; i < strlen(name_to_encode); i++) {
        hpack_huffman_encode_huffman_header_name_value[i] = hpack_huffman_encode_customkey_arr[i];
    }
    for (uint8_t i = strlen(name_to_encode); i < strlen(value_to_encode) + strlen(name_to_encode); i++) {
        hpack_huffman_encode_huffman_header_name_value[i] = hpack_huffman_encode_customvalue_arr[i - strlen(name_to_encode)];
    }
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode, hpack_huffman_encode_huffman_header_name_value, strlen(name_to_encode) + strlen(value_to_encode));

    name_huffman_bool = 1;
    value_huffman_bool = 1;
    rc = encode_literal_header_field_new_name(name_to_encode,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_buffer_huffman);
    TEST_ASSERT_EQUAL(19, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_huffman_string_encoded[i], encoded_buffer_huffman[i]);
    }
}

void test_encode_literal_header_field_new_name_error(void){
    /*Test border condition*/
    char name_to_encode[2 * HTTP2_MAX_HBF_BUFFER];
    char value_to_encode[2 * HTTP2_MAX_HBF_BUFFER];
    uint8_t encoded_string[HTTP2_MAX_HBF_BUFFER];
    memset(encoded_string, 0, HTTP2_MAX_HBF_BUFFER);
    uint8_t name_huffman_bool = 0;
    uint8_t value_huffman_bool = 0;
    hpack_huffman_encode_fake.custom_fake = hpack_huffman_encode_return_w;
    for (int i = 0; i < 2 * HTTP2_MAX_HBF_BUFFER; i++) {
        name_to_encode[i] = 'w';
        value_to_encode[i] = 'w';
    }
    int rc = encode_literal_header_field_new_name(name_to_encode,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_string);
    TEST_ASSERT_EQUAL(-1, rc);
    char name_to_encode2[10];
    for (int i = 0; i < 10; i++) {
        name_to_encode2[i] = 'w';
    }
    rc = encode_literal_header_field_new_name(name_to_encode2,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_string);
    TEST_ASSERT_EQUAL(-1, rc);
    name_huffman_bool = 1;
    value_huffman_bool = 1;

    rc = encode_literal_header_field_new_name(name_to_encode,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_string);
    TEST_ASSERT_EQUAL(-1, rc);

    rc = encode_literal_header_field_new_name(name_to_encode2,  name_huffman_bool, value_to_encode, value_huffman_bool, encoded_string);
    TEST_ASSERT_EQUAL(-1, rc);
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_decode_header_block_literal_without_indexing);
    UNIT_TEST(test_decode_header_block_literal_never_indexed);
    UNIT_TEST(test_encode);

    UNIT_TEST(test_get_preamble);
    UNIT_TEST(test_log128);
    UNIT_TEST(test_encoded_integer_size);
    UNIT_TEST(test_encode_integer);
    UNIT_TEST(test_find_prefix_size);
    UNIT_TEST(test_read_bits_from_bytes);
    UNIT_TEST(test_pack_encoded_words_to_bytes_test1);
    UNIT_TEST(test_pack_encoded_words_to_bytes_test2);
    UNIT_TEST(test_pack_encoded_words_to_bytes_test3);


    UNIT_TEST(test_encode_huffman_word);

    UNIT_TEST(test_encode_non_huffman_string);

    UNIT_TEST(test_encode_huffman_string);


    UNIT_TEST(test_decode_huffman_word);
    UNIT_TEST(test_decode_integer);
    UNIT_TEST(test_decode_non_huffman_string);
    UNIT_TEST(test_decode_huffman_string);
    UNIT_TEST(test_decode_huffman_string_error);
    UNIT_TEST(test_decode_string);
    UNIT_TEST(test_decode_string_error);

    UNIT_TEST(test_encode_then_decode_non_huffman_string);
    UNIT_TEST(test_encode_then_decode_huffman_string);

    UNIT_TEST(test_encode_literal_header_field_new_name);
    UNIT_TEST(test_encode_literal_header_field_new_name_error);
    /*UNIT_TEST(test_frame_to_bytes_settings);
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
