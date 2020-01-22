#include "unit.h"
#include "frames.h"
#include "fff.h"
#include "logging.h"
#include "hpack.h"
#include "hpack/huffman.h"
#include "hpack/tables.h"
#include "hpack/decoder.h"
#include "hpack/utils.h"

#ifdef INCLUDE_HUFFMAN_COMPRESSION
extern int32_t hpack_decoder_decode_huffman_string(char *str, uint32_t str_length, uint8_t *encoded_string, uint32_t encoded_str_length);
extern int32_t hpack_decoder_decode_huffman_word(char *str, uint8_t *encoded_string, uint8_t encoded_string_size, uint16_t bit_position);
#endif
extern int32_t hpack_decoder_decode_string(char *str, uint32_t str_length, uint8_t *encoded_buffer, uint32_t encoded_buffer_length, uint8_t huffman_bit);
extern int32_t hpack_decoder_decode_non_huffman_string(char *str, uint32_t str_length, uint8_t *encoded_string, uint32_t encoded_str_length);
extern int hpack_decoder_decode_indexed_header_field(hpack_dynamic_table_t *dynamic_table,
                                                     hpack_encoded_header_t *encoded_header,
                                                     char *tmp_name,
                                                     char *tmp_value);
extern int32_t hpack_decoder_decode_integer(const uint8_t *bytes, uint8_t prefix);
extern int32_t hpack_decoder_parse_encoded_header(hpack_encoded_header_t *encoded_header, uint8_t *header_block, int32_t header_size);
extern int8_t hpack_check_eos_symbol(uint8_t *encoded_buffer, uint8_t buffer_length);
extern int8_t hpack_decoder_check_errors(hpack_encoded_header_t *encoded_header);
extern hpack_error_t hpack_decoder_check_huffman_padding(uint8_t bits_left, const uint8_t *encoded_buffer, uint32_t str_length);
#if (INCLUDE_HUFFMAN_COMPRESSION == 0)
typedef struct {}huffman_encoded_word_t; /*this is for compilation of hpack_huffman_decode_fake when huffman_compression is not included*/
#endif

/*helper function*/
void pad_code(uint32_t *code, uint8_t length)
{
    uint8_t number_of_padding_bits = 30 - length;
    uint32_t padding = (1 << (number_of_padding_bits)) - 1;
    uint32_t result = *code;

    result <<= number_of_padding_bits;
    result |= padding;
    *code = result;
}

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int8_t, hpack_huffman_decode, huffman_encoded_word_t *, uint8_t *);
FAKE_VALUE_FUNC(int,  header_list_add, header_list_t *, const char *, const char * );
FAKE_VALUE_FUNC(hpack_preamble_t, hpack_utils_get_preamble, uint8_t);
FAKE_VALUE_FUNC(uint8_t, hpack_utils_find_prefix_size, hpack_preamble_t);
FAKE_VALUE_FUNC(int, hpack_encoder_encode, hpack_dynamic_table_t *, char *, char *,  uint8_t *);
FAKE_VALUE_FUNC(uint32_t, hpack_utils_encoded_integer_size, uint32_t, uint8_t);
FAKE_VALUE_FUNC(int8_t, hpack_tables_static_find_name_and_value, uint8_t, char *, char *);
FAKE_VALUE_FUNC(int8_t, hpack_tables_static_find_name, uint8_t, char *);
FAKE_VALUE_FUNC(uint32_t, hpack_tables_get_table_length, uint32_t);
FAKE_VALUE_FUNC(int8_t, hpack_tables_dynamic_table_add_entry, hpack_dynamic_table_t *, char *, char *);
FAKE_VALUE_FUNC(int8_t, hpack_tables_dynamic_table_resize, hpack_dynamic_table_t *, uint32_t);
FAKE_VALUE_FUNC(int8_t, hpack_tables_find_entry_name_and_value, hpack_dynamic_table_t *, uint32_t, char *, char *);
FAKE_VALUE_FUNC(int8_t, hpack_tables_find_entry_name, hpack_dynamic_table_t *, uint32_t, char *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE)                    \
    FAKE(hpack_utils_get_preamble)               \
    FAKE(hpack_utils_find_prefix_size)           \
    FAKE(hpack_encoder_encode)                   \
    FAKE(hpack_utils_encoded_integer_size)       \
    FAKE(hpack_tables_get_table_length)          \
    FAKE(hpack_tables_dynamic_table_add_entry)   \
    FAKE(hpack_tables_find_entry_name_and_value) \
    FAKE(hpack_tables_find_entry_name)           \
    FAKE(hpack_tables_dynamic_table_resize)      \
    FAKE(header_list_add)                            \
    FAKE(hpack_huffman_decode)

/*Decode*/
#if (INCLUDE_HUFFMAN_COMPRESSION)
int8_t hpack_huffman_decode_return_not_found(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    return -1;
}
int8_t hpack_huffman_decode_return_w(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'w';
    //encoded->length = 7;
    return 7;
}
int8_t hpack_huffman_decode_return_dot(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'.';
    //encoded->length = 6;
    return 6;
}
int8_t hpack_huffman_decode_return_e(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'e';
    //encoded->length = 5;
    return 5;
}
int8_t hpack_huffman_decode_return_x(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'x';
    //encoded->length = 7;
    return 7;
}
int8_t hpack_huffman_decode_return_a(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'a';
    //encoded->length = 5;
    return 5;
}
int8_t hpack_huffman_decode_return_m(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'m';
    //encoded->length = 6;
    return 6;
}
int8_t hpack_huffman_decode_return_p(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'p';
    //encoded->length = 6;
    return 6;
}
int8_t hpack_huffman_decode_return_l(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'l';
    //encoded->length = 6;
    return 6;
}
int8_t hpack_huffman_decode_return_c(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'c';
    //encoded->length = 5;
    return 5;
}

int8_t hpack_huffman_decode_return_o(huffman_encoded_word_t *encoded, uint8_t *sym)
{
    *sym = (uint8_t)'o';
    //encoded->length = 5;
    return 5;
}

int8_t(*hpack_huffman_decode_wwwdotexampledotcom_arr[])(huffman_encoded_word_t *, uint8_t *) = { hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_w,
                                                                                                 hpack_huffman_decode_return_dot,
                                                                                                 hpack_huffman_decode_return_e,
                                                                                                 hpack_huffman_decode_return_x,
                                                                                                 hpack_huffman_decode_return_a,
                                                                                                 hpack_huffman_decode_return_m,
                                                                                                 hpack_huffman_decode_return_p,
                                                                                                 hpack_huffman_decode_return_l,
                                                                                                 hpack_huffman_decode_return_e,
                                                                                                 hpack_huffman_decode_return_dot,
                                                                                                 hpack_huffman_decode_return_c,
                                                                                                 hpack_huffman_decode_return_o,
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
#endif

int8_t hpack_tables_find_name_return_authority(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    (void)index;
    char authority[] = ":authority";
    strncpy(name, authority, strlen(authority));
    return 0;
}

int8_t hpack_tables_find_name_return_age(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    (void)index;
    char age[] = "age";
    strncpy(name, age, strlen(age));
    return 0;
}

int8_t hpack_tables_find_name_return_new_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    (void)index;
    char age[] = "new_name";
    strncpy(name, age, strlen(age));
    return 0;
}

int8_t hpack_tables_find_entry_name_and_value_return_method_get(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value)
{
    TEST_ASSERT_EQUAL(2, index);
    strncpy(name, ":method", 8);
    strncpy(value, "GET", 4);
    return 0;
}

void hpack_tables_init_dynamic_table_custom_fake(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size)
{
    memset(dynamic_table->buffer, 0, dynamic_table_max_size);
    dynamic_table->max_size = dynamic_table->settings_max_table_size = (uint16_t)dynamic_table_max_size;
    dynamic_table->actual_size = 0;
    dynamic_table->n_entries = 0;
    dynamic_table->first = 0;
    dynamic_table->next = 0;
}

int header_list_add_check_inputs(header_list_t *headers, const char *name, const char *value)
{
    TEST_ASSERT_EQUAL(0, memcmp("new_name", name, strlen("new_name")));
    TEST_ASSERT_EQUAL(0, memcmp("val", value, strlen("val")));
    return 0;
}

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_hpack_decoder_check_huffman_padding(void)
{
    /*Test with correct padding*/
    uint16_t bits_left = 7;
    uint8_t encoded_buffer[2];

    encoded_buffer[0] = 0x7f;
    encoded_buffer[1] = 0;
    uint32_t str_length = 1;
    int32_t rc = hpack_decoder_check_huffman_padding(bits_left, encoded_buffer, str_length);
    TEST_ASSERT_EQUAL(0, rc); //ok
    /*Test with wrong padding*/
    encoded_buffer[0] = 0x5f;
    rc = hpack_decoder_check_huffman_padding(bits_left, encoded_buffer, str_length);
    TEST_ASSERT_EQUAL(-1, rc);
    /**/
}

void test_hpack_decoder_check_eos_symbol(void)
{
    uint8_t encoded_buffer[] = { 0x3f, 0xff, 0xff, 0xff };
    uint8_t buffer_length = 4;

    int8_t rc = hpack_check_eos_symbol(encoded_buffer, buffer_length);
    TEST_ASSERT_EQUAL(-1, rc);

    /*test with a shift*/
    setUp();
    rc = hpack_check_eos_symbol(encoded_buffer, buffer_length);
    TEST_ASSERT_EQUAL(-1, rc);

}

void test_hpack_decoder_hpack_decoder_check_errors(void)
{
    hpack_encoded_header_t encoded_header;

    /*Row doesn't exist*/
    encoded_header.preamble = INDEXED_HEADER_FIELD;
    encoded_header.index = 0;

    int8_t rc = hpack_decoder_check_errors(&encoded_header);
    TEST_ASSERT_EQUAL(-1, rc);

    /*Integer exceed implementations limits in dynamic table size update*/
    encoded_header.preamble = DYNAMIC_TABLE_SIZE_UPDATE;
    encoded_header.dynamic_table_size = 2 * HPACK_MAXIMUM_INTEGER;
    rc = hpack_decoder_check_errors(&encoded_header);
    TEST_ASSERT_EQUAL(-1, rc);

    /*Test an ok INDEXED HEADER FIELD*/
    encoded_header.preamble = INDEXED_HEADER_FIELD;
    encoded_header.index = 1;
    rc = hpack_decoder_check_errors(&encoded_header);
    TEST_ASSERT_EQUAL(0, rc);

    /*Test an ok DYNAMIC TABLE SIZE*/
    encoded_header.preamble = DYNAMIC_TABLE_SIZE_UPDATE;
    encoded_header.dynamic_table_size = HPACK_MAXIMUM_INTEGER;
    rc = hpack_decoder_check_errors(&encoded_header);
    TEST_ASSERT_EQUAL(0, rc);

    /*Test a LITERAL HEADER FIELD with an EOS symbol in the name*/
    uint8_t encoded_buffer[] = { 0x3f, 0xff, 0xff, 0xff };

    encoded_header.index = 0;
    encoded_header.preamble = LITERAL_HEADER_FIELD_NEVER_INDEXED;
    encoded_header.name_length = 4;
    encoded_header.name_string = encoded_buffer;

    rc = hpack_decoder_check_errors(&encoded_header);

    TEST_ASSERT_EQUAL(-1, rc);

    /*Test a LITERAL HEADER FIELD with an EOS symbol in the value*/
    encoded_header.index = 1;
    encoded_header.value_length = 4;
    encoded_header.value_string = encoded_buffer;

    rc = hpack_decoder_check_errors(&encoded_header);

    TEST_ASSERT_EQUAL(-1, rc);
}


void test_parse_encoded_header_test1(void)
{
    //Literal Header Field Representation
    //with incremental indexing
    //No huffman encoding - Header name as string literal
    uint8_t header_block_size = 26;
    uint8_t header_block_name_literal[] = { 0x40,
                                            0x0a,
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
                                            0x0d,
                                            0x63,
                                            0x75,
                                            0x73,
                                            0x74,
                                            0x6f,
                                            0x6d,
                                            0x2d,
                                            0x68,
                                            0x65,
                                            0x61,
                                            0x64,
                                            0x65,
                                            0x72 };
    hpack_encoded_header_t encoded_header;
    char *expected_name = "custom-key";
    char *expected_value = "custom-header";

    hpack_utils_encoded_integer_size_fake.return_val = 1;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)64;
    hpack_utils_find_prefix_size_fake.return_val = 6;
    int8_t rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name_literal, header_block_size);
    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(0, encoded_header.index);
    TEST_ASSERT_EQUAL(10, encoded_header.name_length);
    TEST_ASSERT_EQUAL(13, encoded_header.value_length);
    TEST_ASSERT_EQUAL(0, encoded_header.huffman_bit_of_name);
    TEST_ASSERT_EQUAL(0, encoded_header.huffman_bit_of_value);
    for (uint8_t i = 0; i < strlen(expected_name); i++) {
        TEST_ASSERT_EQUAL(expected_name[i], encoded_header.name_string[i]);
    }
    for (uint8_t i = 0; i < strlen(expected_value); i++) {
        TEST_ASSERT_EQUAL(expected_value[i], encoded_header.value_string[i]);
    }
}

void test_parse_encoded_header_test2(void)
{
    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as string literal
    uint8_t header_block_size = 10;
    uint8_t header_block_name_literal[] = {
        16,         //00010000 prefix=0001, index=0
        4,          //h=0, name length = 4;
        'h',        //name string
        'o',        //name string
        'l',        //name string
        'a',        //name string
        3,          //h=0, value length = 3;
        'v',        //value string
        'a',        //value string
        'l'         //value string
    };
    char *expected_name = "hola";
    char *expected_value = "val";

    hpack_utils_encoded_integer_size_fake.return_val = 1;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)16;
    hpack_utils_find_prefix_size_fake.return_val = 4;

    hpack_encoded_header_t encoded_header;

    int8_t rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name_literal, header_block_size);
    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_NEVER_INDEXED, encoded_header.preamble);
    TEST_ASSERT_EQUAL(0, encoded_header.index);
    TEST_ASSERT_EQUAL(4, encoded_header.name_length);
    TEST_ASSERT_EQUAL(3, encoded_header.value_length);
    TEST_ASSERT_EQUAL(0, encoded_header.huffman_bit_of_name);
    TEST_ASSERT_EQUAL(0, encoded_header.huffman_bit_of_value);

    for (uint8_t i = 0; i < strlen(expected_name); i++) {
        TEST_ASSERT_EQUAL(expected_name[i], encoded_header.name_string[i]);
    }
    for (uint8_t i = 0; i < strlen(expected_value); i++) {
        TEST_ASSERT_EQUAL(expected_value[i], encoded_header.value_string[i]);
    }

    /*Indexed name*/
    header_block_size = 5;
    uint8_t header_block_name_indexed[] = {
        17,         //00010001 prefix=0001, index=1
        3,          //h=0, value length = 3;
        'v',
        'a',
        'l'
    };
    expected_value = "val";
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name_indexed, header_block_size);
    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_NEVER_INDEXED, encoded_header.preamble);
    TEST_ASSERT_EQUAL(1, encoded_header.index);
    TEST_ASSERT_EQUAL(3, encoded_header.value_length);
    TEST_ASSERT_EQUAL(0, encoded_header.huffman_bit_of_value);

    for (uint8_t i = 0; i < strlen(expected_value); i++) {
        TEST_ASSERT_EQUAL(expected_value[i], encoded_header.value_string[i]);
    }
}

void test_parse_encoded_header_test3(void)
{
    //Indexed header field
    //index is 2
    uint8_t header_block_size = 1;
    uint8_t header_block_literal[] = { 0x82 };

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    hpack_utils_find_prefix_size_fake.return_val = 7;

    hpack_encoded_header_t encoded_header;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)128;

    int8_t rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_literal, header_block_size);
    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(INDEXED_HEADER_FIELD, encoded_header.preamble);
    TEST_ASSERT_EQUAL(2, encoded_header.index);

    //Dynamic table update
    //index is 2
    header_block_size = 2;
    uint8_t header_block_literal2[] = { 0x3f, 0x6e };
    hpack_utils_encoded_integer_size_fake.return_val = 2;
    hpack_utils_find_prefix_size_fake.return_val = 5;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)32;

    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_literal2, header_block_size);
    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(DYNAMIC_TABLE_SIZE_UPDATE, encoded_header.preamble);
    TEST_ASSERT_EQUAL(141, encoded_header.dynamic_table_size);

}

void test_hpack_decoder_parse_encoded_header_error(void)
{
    //Literal Header Field Representation
    //with incremental indexing
    //No huffman encoding

    /*Encoded a 4097*/
    uint8_t header_block_name1[] = { 0x7f,  //63
                                     0xc2,  //194
                                     0x1f,  //31
    };
    hpack_encoded_header_t encoded_header;
    uint8_t header_size = 3;

    hpack_utils_get_preamble_fake.return_val = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;

    hpack_utils_encoded_integer_size_fake.return_val = 3;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)64;
    hpack_utils_find_prefix_size_fake.return_val = 6;

    int8_t rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name1, header_size);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);

    /*Test a header block size smaller than needed*/
    /*Encoded a 4093 index*/
    uint8_t header_block_name2[] = { 0x7f,  //63
                                     0xbe,  //190
                                     0x1f,  //31
    };
    hpack_utils_encoded_integer_size_fake.return_val = 4;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name2, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(4093, encoded_header.index);

    /*Test a header block with name length greater than 4096*/
    /*Encoding a name length of 4097*/
    uint8_t header_block_name3[] = { 0x40,  //32
                                     0x7f,  //127
                                     0x82,  //130
                                     0x1f,  //31
    };
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    header_size = 4;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name3, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(0, encoded_header.index);
    /*Test a header block with less bytes than needed*/
    /*Encoding a name length of 4096*/
    uint8_t header_block_name4[] = { 0x40,  //32
                                     0x7f,  //127
                                     0x81,  //129
                                     0x1f,  //31
    };
    header_size = 1;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name4, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(0, encoded_header.index);
    TEST_ASSERT_EQUAL(4096, encoded_header.name_length);
    /*Test a header block with less bytes than needed in name_length*/
    /*Encoding a name length of 4096 same as before*/
    uint8_t header_block_name5[] = { 0x40,  //32
                                     0x7f,  //127
                                     0x81,  //129
                                     0x1f,  //31
    };
    header_size = 2;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_name5, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(0, encoded_header.index);
    TEST_ASSERT_EQUAL(4096, encoded_header.name_length);
    /*Test with value*/
    /*Test a header block with value length greater than 4096*/
    /*Encoding a value length of 4097*/
    uint8_t header_block_value1[] = { 0x41, //33, index = 1
                                      0x7f, //127
                                      0x82, //130
                                      0x1f, //31
    };
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    header_size = 4;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_value1, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(1, encoded_header.index);

    /*Test a header block with less bytes than needed*/
    /*Encoding a value length of 4096*/
    uint8_t header_block_value2[] = { 0x41, //33, index = 1
                                      0x7f, //127
                                      0x81, //129
                                      0x1f, //31
    };
    header_size = 1;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_value2, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(1, encoded_header.index);
    TEST_ASSERT_EQUAL(4096, encoded_header.value_length);

    /*Test a header block with less bytes than needed in name_length*/
    /*Encoding a value length of 4096 same as before*/
    uint8_t header_block_value3[] = { 0x41, //33, index = 1
                                      0x7f, //127
                                      0x81, //129
                                      0x1f, //31
    };
    header_size = 2;
    rc = hpack_decoder_parse_encoded_header(&encoded_header, header_block_value3, header_size);

    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING, encoded_header.preamble);
    TEST_ASSERT_EQUAL(1, encoded_header.index);
    TEST_ASSERT_EQUAL(4096, encoded_header.value_length);
}

#if HPACK_INCLUDE_DYNAMIC_TABLE
void test_decode_header_literal_with_incremental_indexing(void)
{
    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as string literal
    uint8_t header_block_size = 26;
    uint8_t header_block_name_literal[] = { 0x40,
                                            0x0a,
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
                                            0x0d,
                                            0x63,
                                            0x75,
                                            0x73,
                                            0x74,
                                            0x6f,
                                            0x6d,
                                            0x2d,
                                            0x68,
                                            0x65,
                                            0x61,
                                            0x64,
                                            0x65,
                                            0x72 };
    char *expected_name = "custom-key";
    char *expected_value = "custom-header";

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    //uint32_t hpack_encoded_integer_size_fake_seq[] = { 1, 1, 1, 1, 2, 1 };
    //SET_RETURN_SEQ(hpack_utils_encoded_integer_size, hpack_encoded_integer_size_fake_seq, 6);

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)64;
    hpack_utils_find_prefix_size_fake.return_val = 6;

    uint32_t max_dynamic_table_size = 200;

    hpack_dynamic_table_t dynamic_table;
    hpack_tables_init_dynamic_table_custom_fake(&dynamic_table, max_dynamic_table_size);
    char name[11];
    char value[14];
    memset(name, 0, 11);
    memset(value, 0, 14);


    header_list_t headers;

    headers.count = 0;
    headers.size = 0;
    //int rc = hpack_decoder_decode_literal_header_field_with_incremental_indexing(&dynamic_table, header_block_name_literal, name, value);
    int rc = hpack_decoder_decode(&dynamic_table, header_block_name_literal, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded

    for (uint8_t i = 0; i < strlen(name); i++) {
        TEST_ASSERT_EQUAL(expected_name[i], name[i]);
    }
    for (uint8_t i = 0; i < strlen(value); i++) {
        TEST_ASSERT_EQUAL(expected_value[i], value[i]);
    }
}
#endif

void test_decode_header_literal_never_indexed(void)
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

    header_list_t headers;

    hpack_dynamic_table_t dynamic_table;
    hpack_tables_init_dynamic_table_custom_fake(&dynamic_table, 100); // 100 is a dummy value

    headers.count = 0;
    headers.size = 0;

    uint32_t hpack_encoded_integer_size_fake_seq[] = {
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        1,
        2,
        1,
        2,
        1
    };

    //uint32_t hpack_encoded_integer_size_fake_seq[] = { 1, 1, 1, 1, 2, 1 };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size, hpack_encoded_integer_size_fake_seq, 14);

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)16;
    hpack_utils_find_prefix_size_fake.return_val = 4;

    int rc = hpack_decoder_decode(&dynamic_table, header_block_name_literal, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(1, header_list_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, header_list_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, header_list_add_fake.arg2_val);

    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as static table index

    headers.count = 0;
    headers.size = 0;

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
    rc = hpack_decoder_decode(&dynamic_table, header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, header_list_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, header_list_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, header_list_add_fake.arg2_val);

    //Literal Header Field Representation
    //Never indexed
    //No huffman encoding - Header name as dynamic table index

    uint32_t max_dynamic_table_size = 200;

    hpack_tables_init_dynamic_table_custom_fake(&dynamic_table, max_dynamic_table_size);
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

    header_list_add_fake.custom_fake = header_list_add_check_inputs;
    rc = hpack_decoder_decode(&dynamic_table, header_block_dynamic_index, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);
    TEST_ASSERT_EQUAL(3, header_list_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(new_name, header_list_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, header_list_add_fake.arg2_val);

}

void test_decode_header_literal_without_indexing(void)
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

    header_list_t headers;

    hpack_dynamic_table_t dynamic_table;
    hpack_tables_init_dynamic_table_custom_fake(&dynamic_table, 100); //100 is a dummy value btw

    headers.count = 0;
    headers.size = 0;

    hpack_utils_get_preamble_fake.return_val = (hpack_preamble_t)0;
    hpack_utils_find_prefix_size_fake.return_val = 4;

    uint32_t hpack_encoded_integer_size_fake_seq[] = {
        1,
        1,
        1,
        1,
        1,
        2,
        1,
        2,
        1,
    };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size, hpack_encoded_integer_size_fake_seq, 9);


    int rc = hpack_decoder_decode(&dynamic_table, header_block_name_literal, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(1, header_list_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, header_list_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, header_list_add_fake.arg2_val);

    //Literal Header Field Representation
    //without indexing
    //No huffman encoding - Header name as static table index

    headers.count = 0;
    headers.size = 0;

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
    rc = hpack_decoder_decode(&dynamic_table, header_block_name_indexed, header_block_size, &headers);

    TEST_ASSERT_EQUAL(header_block_size, rc);//bytes decoded
    TEST_ASSERT_EQUAL(2, header_list_add_fake.call_count);
    TEST_ASSERT_EQUAL_STRING(expected_name, header_list_add_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(expected_value, header_list_add_fake.arg2_val);


}

void test_decode_non_huffman_string(void)
{
    uint8_t encoded_string[] = { 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    int str_length = 15;
    char decoded_string[str_length];

    hpack_utils_encoded_integer_size_fake.return_val = 1;

    memset(decoded_string, 0, str_length);
    char expected_decoded_string[] = "www.example.com";
    int rc = hpack_decoder_decode_non_huffman_string(decoded_string, str_length, encoded_string, str_length);
    TEST_ASSERT_EQUAL(rc, 16);
    for (int i = 0; i < str_length; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
}


#if (INCLUDE_HUFFMAN_COMPRESSION)
void test_decode_huffman_word(void)
{
    uint8_t *encoded_string = encoded_wwwdotexampledotcom + 1; //We don't need to decode as string the first byte
    char expected_decoded_string[] = "www.example.com";
    uint8_t expected_word_length[] = { 7, 7, 7, 6, 5, 7, 5, 6, 6, 6, 5, 6, 5, 5, 6 };

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 16);

    char decoded_sym = (char)0;
    uint8_t encoded_string_size = 12;
    uint16_t bit_position = 0;

    for (int i = 0; i < 15; i++) {
        int8_t rc = hpack_decoder_decode_huffman_word(&decoded_sym, encoded_string, encoded_string_size, bit_position);
        TEST_ASSERT_EQUAL(expected_word_length[i], rc);
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_sym);
        bit_position += rc;
    }
    TEST_ASSERT_EQUAL(15, hpack_huffman_decode_fake.call_count);
}
#endif

#if (INCLUDE_HUFFMAN_COMPRESSION)
void test_decode_huffman_string(void)
{
    uint8_t str_length = 30;
    char decoded_string[str_length];

    memset(decoded_string, 0, str_length);
    char expected_decoded_string[] = "www.example.com";
    uint8_t *encoded_string = encoded_wwwdotexampledotcom + 1;

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
    int rc = hpack_decoder_decode_huffman_string(decoded_string, str_length, encoded_string, 12);
    TEST_ASSERT_EQUAL(13, rc);
    TEST_ASSERT_EQUAL(16, hpack_huffman_decode_fake.call_count);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
}
#endif

#if (INCLUDE_HUFFMAN_COMPRESSION)
void test_decode_huffman_string_error(void)
{
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_bad_padding_arr, 2);
    hpack_utils_encoded_integer_size_fake.return_val = 1;

    /*Test border condition*/
    /*Padding wrong*/
    uint8_t encoded_string2[] = { 0x1b };
    char expected_decoded_string2[] = "a";
    char decoded_string2[] = { 0, 0 };

    int rc = hpack_decoder_decode_huffman_string(decoded_string2, 2, encoded_string2, 1);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(1, hpack_huffman_decode_fake.call_count);
    TEST_ASSERT_EQUAL(expected_decoded_string2[0], decoded_string2[0]);

    /*Couldn't read code*/
    uint8_t encoded_string4[] = { 0x81, 0x6f };
    char decoded_string4[] = { 0, 0 };
    char expected_decoded_string4[] = { 0, 0 };
    rc = hpack_decoder_decode_huffman_string(decoded_string4, 2, encoded_string4, 2);
    TEST_ASSERT_EQUAL(-1, rc);
    for (int i = 0; i < 2; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string4[i], decoded_string4[i]);
    }

    /*Encoding the EOS symbol*/
    uint8_t encoded_string5[] = { 0x84, 0x7f, 0xff, 0xff, 0xff };
    char decoded_string5[] = { 0, 0, 0, 0 };
    rc = hpack_decoder_decode_huffman_string(decoded_string5, 4, encoded_string5, 5);
    TEST_ASSERT_EQUAL(-1, rc);
}
#endif

void test_decode_string(void)
{
    uint8_t encoded_string[] = { 0x77, 0x77, 0x77, 0x2e, 0x65, 0x78, 0x61, 0x6d, 0x70, 0x6c, 0x65, 0x2e, 0x63, 0x6f, 0x6d };
    int str_length = 15;
    char decoded_string[str_length];

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    /*Test decode a non huffman string*/
    memset(decoded_string, 0, str_length);
    char expected_decoded_string[] = "www.example.com";
    int rc = hpack_decoder_decode_string(decoded_string, str_length, encoded_string, str_length, 0);
    TEST_ASSERT_EQUAL(16, rc);
    for (int i = 0; i < str_length; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
    #if (INCLUDE_HUFFMAN_COMPRESSION)
    /*Test decode a huffman string*/
    memset(decoded_string, 0, str_length);

    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_wwwdotexampledotcom_arr, 30);
    uint8_t *encoded_string_huffman = encoded_wwwdotexampledotcom + 1;
    int rc2 = hpack_decoder_decode_string(decoded_string, str_length, encoded_string_huffman, 12, 1);
    TEST_ASSERT_EQUAL(16, hpack_huffman_decode_fake.call_count);

    TEST_ASSERT_EQUAL(13, rc2);
    for (int i = 0; i < rc2; i++) {
        TEST_ASSERT_EQUAL(expected_decoded_string[i], decoded_string[i]);
    }
    #endif
}

#if (INCLUDE_HUFFMAN_COMPRESSION)
void test_decode_string_error(void)
{
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_decode, hpack_huffman_decode_bad_padding_arr, 2);

    /*Test border condition*/
    /*Padding wrong*/
    uint8_t encoded_string2[] = { 0x1b };
    char expected_decoded_string2[] = "a";
    char decoded_string2[] = { 0, 0 };

    int rc = hpack_decoder_decode_string(decoded_string2, 2, encoded_string2, 1, 1);
    TEST_ASSERT_EQUAL(-1, rc);
    TEST_ASSERT_EQUAL(expected_decoded_string2[0], decoded_string2[0]);
}
#endif

void test_decode_integer(void)
{
    uint8_t bytes1[] = {
        30    //000 11110
    };
    uint8_t prefix = 5;
    uint32_t rc = hpack_decoder_decode_integer(bytes1, prefix);

    TEST_ASSERT_EQUAL(30, rc);

    uint8_t bytes2[] = {
        31,     //000 11111
        0       //+
    };
    rc = hpack_decoder_decode_integer(bytes2, prefix);
    TEST_ASSERT_EQUAL(31, rc);

    uint8_t bytes3[] = {
        31,     //000 11111
        1,      //
    };
    rc = hpack_decoder_decode_integer(bytes3, prefix);
    TEST_ASSERT_EQUAL(32, rc);

    uint8_t bytes4[] = {
        31,     //000 11111
        2,      //
    };
    rc = hpack_decoder_decode_integer(bytes4, prefix);
    TEST_ASSERT_EQUAL(33, rc);

    uint8_t bytes5[] = {
        31,     //000 11111
        128,    //
        1
    };
    rc = hpack_decoder_decode_integer(bytes5, prefix);
    TEST_ASSERT_EQUAL(128 + 31, rc);

    uint8_t bytes6[] = {
        31,     //000 11111
        154,    //
        10
    };
    rc = hpack_decoder_decode_integer(bytes6, prefix);
    TEST_ASSERT_EQUAL(1337, rc);
}

void test_hpack_decoder_decode_indexed_header_field(void)
{

    char expected_name[] = ":method\0";
    char expected_value[] = "GET\0";


    hpack_dynamic_table_t dynamic_table;
    hpack_tables_init_dynamic_table_custom_fake(&dynamic_table, 100);     //100 is a dummy value btw

    char name[HPACK_HEADER_NAME_LEN];
    memset(name, 0, HPACK_HEADER_NAME_LEN);
    char value[HPACK_HEADER_VALUE_LEN];
    memset(value, 0, HPACK_HEADER_VALUE_LEN);

    hpack_encoded_header_t encoded_header;
    encoded_header.index = 2;
    encoded_header.preamble = INDEXED_HEADER_FIELD;

    hpack_tables_find_entry_name_and_value_fake.custom_fake = hpack_tables_find_entry_name_and_value_return_method_get;
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    hpack_utils_find_prefix_size_fake.return_val = 7;
    int rc = hpack_decoder_decode_indexed_header_field(&dynamic_table,
                                                       &encoded_header,
                                                       name,
                                                       value);

    TEST_ASSERT_EQUAL(1, rc);
    for (uint8_t i = 0; i < strlen(expected_name); i++) {
        TEST_ASSERT_EQUAL(expected_name[i], name[i]);
    }
    for (uint8_t i = 0; i < strlen(expected_value); i++) {
        TEST_ASSERT_EQUAL(expected_value[i], value[i]);
    }
    //Test error
    setUp();
    hpack_tables_find_entry_name_and_value_fake.return_val = -1;
    rc = hpack_decoder_decode_indexed_header_field(&dynamic_table,
                                                   &encoded_header,
                                                   name,
                                                   value);
    TEST_ASSERT_EQUAL(-1, rc);

}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_decode_header_literal_without_indexing);
    UNIT_TEST(test_decode_header_literal_never_indexed);
#if (HPACK_INCLUDE_DYNAMIC_TABLE)
    UNIT_TEST(test_decode_header_literal_with_incremental_indexing);
#endif
    UNIT_TEST(test_hpack_decoder_decode_indexed_header_field);
    UNIT_TEST(test_parse_encoded_header_test1);
    UNIT_TEST(test_parse_encoded_header_test2);
    UNIT_TEST(test_parse_encoded_header_test3);
    UNIT_TEST(test_hpack_decoder_parse_encoded_header_error);
    UNIT_TEST(test_hpack_decoder_check_huffman_padding);
    UNIT_TEST(test_hpack_decoder_check_eos_symbol);
    UNIT_TEST(test_hpack_decoder_hpack_decoder_check_errors);
#if (INCLUDE_HUFFMAN_COMPRESSION)
    UNIT_TEST(test_decode_huffman_word);
    UNIT_TEST(test_decode_huffman_string);
    UNIT_TEST(test_decode_huffman_string_error);
    UNIT_TEST(test_decode_string_error);
#endif

    UNIT_TEST(test_decode_integer);
    UNIT_TEST(test_decode_non_huffman_string);
    UNIT_TEST(test_decode_string);

    return UNIT_TESTS_END();
}
