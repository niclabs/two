#include "fff.h"
#include "hpack.h"
#include "hpack/encoder.h"
#include "hpack/huffman.h"
#include "logging.h"
#include "unit.h"

extern int8_t hpack_encoder_pack_encoded_words_to_bytes(
  huffman_encoded_word_t *encoded_words,
  uint32_t encoded_words_size,
  uint8_t *buffer,
  uint8_t buffer_size);
extern uint32_t hpack_encoder_encode_huffman_word(
  char *str,
  uint32_t str_length,
  huffman_encoded_word_t *encoded_words);
extern int hpack_encoder_encode_huffman_string(char *str,
                                               uint8_t *encoded_string,
                                               uint32_t buffer_size);
extern int hpack_encoder_encode_non_huffman_string(char *str,
                                                   uint8_t *encoded_string,
                                                   uint32_t buffer_size);
extern int hpack_encoder_encode_integer(uint32_t integer,
                                        uint8_t prefix,
                                        uint8_t *encoded_integer);
extern int hpack_encoder_encode_dynamic_size_update(
  hpack_dynamic_table_t *dynamic_table,
  uint32_t max_size,
  uint8_t *encoded_buffer);
extern int hpack_encoder_encode_header(hpack_encoded_header_t *encoded_header,
                                       char *name_string,
                                       char *value_string,
                                       uint8_t *encoded_buffer,
                                       uint32_t buffer_size);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int8_t,
                hpack_huffman_encode,
                huffman_encoded_word_t *,
                uint8_t);
FAKE_VALUE_FUNC(uint8_t, hpack_utils_find_prefix_size, hpack_preamble_t);
FAKE_VALUE_FUNC(uint32_t, hpack_utils_encoded_integer_size, uint32_t, uint8_t);
FAKE_VALUE_FUNC(int,
                hpack_tables_find_index,
                hpack_dynamic_table_t *,
                char *,
                char *);
FAKE_VALUE_FUNC(int,
                hpack_tables_find_index_name,
                hpack_dynamic_table_t *,
                char *);
FAKE_VALUE_FUNC(int8_t,
                hpack_tables_dynamic_table_add_entry,
                hpack_dynamic_table_t *,
                char *,
                char *);
FAKE_VALUE_FUNC(int8_t,
                hpack_tables_dynamic_table_resize,
                hpack_dynamic_table_t *,
                uint32_t);

FAKE_VALUE_FUNC(unsigned int, header_list_count, header_list_t *);
FAKE_VALUE_FUNC(http_header_t *,
                header_list_all,
                header_list_t *,
                http_header_t *);

#define FFF_FAKES_LIST(FAKE)                                                   \
    FAKE(hpack_utils_find_prefix_size)                                         \
    FAKE(hpack_utils_encoded_integer_size)                                     \
    FAKE(hpack_tables_find_index)                                              \
    FAKE(hpack_tables_find_index_name)                                         \
    FAKE(hpack_tables_dynamic_table_add_entry)                                 \
    FAKE(hpack_tables_dynamic_table_resize)                                    \
    FAKE(hpack_huffman_encode)                                                 \
    FAKE(header_list_count)                                                    \
    FAKE(header_list_all)

/*----------Value Return for FAKEs ----------*/
int8_t hpack_huffman_encode_return_w(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x78;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_dot(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x17;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_a(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x3;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_e(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x5;
    h->length = 5;
    return 0;
}

int8_t hpack_huffman_encode_return_x(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x79;
    h->length = 7;
    return 0;
}

int8_t hpack_huffman_encode_return_m(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x29;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_p(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x2b;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_l(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x28;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_c(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x4;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_o(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x7;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_u(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x2d;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_s(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x8;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_t(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x9;
    h->length = 5;
    return 0;
}
int8_t hpack_huffman_encode_return_minus(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x16;
    h->length = 6;
    return 0;
}
int8_t hpack_huffman_encode_return_k(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x75;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_y(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x7a;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_v(huffman_encoded_word_t *h, uint8_t sym)
{
    h->code   = 0x77;
    h->length = 7;
    return 0;
}
int8_t hpack_huffman_encode_return_infinity(huffman_encoded_word_t *h,
                                            uint8_t sym)
{
    h->code   = 0x0;
    h->length = 255;
    return 0;
}
uint8_t encoded_wwwdotexampledotcom[] = { 0x8c, 0xf1, 0xe3, 0xc2, 0xe5,
                                          0xf2, 0x3a, 0x6b, 0xa0, 0xab,
                                          0x90, 0xf4, 0xff };
int8_t (*hpack_huffman_encode_wwwdotexampledotcom_arr[])(
  huffman_encoded_word_t *,
  uint8_t) = { hpack_huffman_encode_return_w, hpack_huffman_encode_return_w,
               hpack_huffman_encode_return_w, hpack_huffman_encode_return_dot,
               hpack_huffman_encode_return_e, hpack_huffman_encode_return_x,
               hpack_huffman_encode_return_a, hpack_huffman_encode_return_m,
               hpack_huffman_encode_return_p, hpack_huffman_encode_return_l,
               hpack_huffman_encode_return_e, hpack_huffman_encode_return_dot,
               hpack_huffman_encode_return_c, hpack_huffman_encode_return_o,
               hpack_huffman_encode_return_m };

int8_t (*hpack_huffman_encode_customkey_arr[])(huffman_encoded_word_t *,
                                               uint8_t) = {
    hpack_huffman_encode_return_c,     hpack_huffman_encode_return_u,
    hpack_huffman_encode_return_s,     hpack_huffman_encode_return_t,
    hpack_huffman_encode_return_o,     hpack_huffman_encode_return_m,
    hpack_huffman_encode_return_minus, hpack_huffman_encode_return_k,
    hpack_huffman_encode_return_e,     hpack_huffman_encode_return_y
};

int8_t (*hpack_huffman_encode_customvalue_arr[])(huffman_encoded_word_t *,
                                                 uint8_t) = {
    hpack_huffman_encode_return_c,     hpack_huffman_encode_return_u,
    hpack_huffman_encode_return_s,     hpack_huffman_encode_return_t,
    hpack_huffman_encode_return_o,     hpack_huffman_encode_return_m,
    hpack_huffman_encode_return_minus, hpack_huffman_encode_return_v,
    hpack_huffman_encode_return_a,     hpack_huffman_encode_return_l,
    hpack_huffman_encode_return_u,     hpack_huffman_encode_return_e
};

int hpack_tables_find_index_fake_method_get(
  hpack_dynamic_table_t *dynamic_table,
  char *name,
  char *value)
{
    TEST_ASSERT_EQUAL_STRING(":method", name);
    TEST_ASSERT_EQUAL_STRING("GET", value);
    return 2;
}

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_encode_integer(void)
{

    uint32_t integer                                     = 10;
    uint8_t prefix                                       = 5;
    uint32_t hpack_utils_encoded_integer_size_fake_seq[] = { 1, 1, 1, 1, 3, 3,
                                                             2, 2, 1, 1, 3, 3 };

    SET_RETURN_SEQ(hpack_utils_encoded_integer_size,
                   hpack_utils_encoded_integer_size_fake_seq,
                   12);
    int rc = hpack_utils_encoded_integer_size(integer, prefix);
    uint8_t encoded_integer[4];

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes1[] = {
        10 // 0,0,0, 0,1,0,1,0
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes1[i], encoded_integer[i]);
    }

    integer = 30;
    prefix  = 5;
    rc      = hpack_utils_encoded_integer_size(integer, prefix);

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes2[] = {
        30 // 0,0,0, 1,1,1,1,0
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes2[i], encoded_integer[i]);
    }

    integer = 128 + 31;
    prefix  = 5;
    rc      = hpack_utils_encoded_integer_size(integer, prefix);

    for (int i = 0; i < 5; i++) {
        encoded_integer[i] = 0;
    }

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(3, rc);
    uint8_t expected_bytes6[] = { 31, // 0,0,0, 1,1,1,1,1
                                  128,
                                  1 };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes6[i], encoded_integer[i]);
    }

    integer = 31;
    prefix  = 5;
    rc      = hpack_utils_encoded_integer_size(integer, prefix);

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(2, rc);
    uint8_t expected_bytes3[] = {
        31, // 0,0,0, 1,1,1,1,1
        0   // 0 000 0000
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes3[i], encoded_integer[i]);
    }

    integer = 31;
    prefix  = 6;
    rc      = hpack_utils_encoded_integer_size(integer, prefix);

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(1, rc);
    uint8_t expected_bytes4[] = {
        31 // 0,0, 0,1,1,1,1,1
    };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes4[i], encoded_integer[i]);
    }

    integer = 1337;
    prefix  = 5;
    rc      = hpack_utils_encoded_integer_size(integer, prefix);

    rc = hpack_encoder_encode_integer(integer, prefix, encoded_integer);
    TEST_ASSERT_EQUAL(3, rc);
    uint8_t expected_bytes5[] = { 31, // 0,0,0, 1,1,1,1,1
                                  154,
                                  10 };
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_bytes5[i], encoded_integer[i]);
    }
}

void test_encode_non_huffman_string(void)
{
    char str[] = "char_to_encode"; // 14
    uint8_t encoded_string[30];
    uint8_t expected_encoded_string[] = { 14,  'c', 'h', 'a', 'r',
                                          '_', 't', 'o', '_', 'e',
                                          'n', 'c', 'o', 'd', 'e' };

    hpack_utils_encoded_integer_size_fake.return_val = 1;

    int rc = hpack_encoder_encode_non_huffman_string(str, encoded_string, 30);
    TEST_ASSERT_EQUAL(15, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_string[i], encoded_string[i]);
    }
}

void test_pack_encoded_words_to_bytes_test1(void)
{
    huffman_encoded_word_t encoded_buffer[] = {
        /*www.example.com*/
        { .code = 0x78, .length = 7 }, { .code = 0x78, .length = 7 },
        { .code = 0x78, .length = 7 }, { .code = 0x17, .length = 6 },
        { .code = 0x5, .length = 5 },  { .code = 0x79, .length = 7 },
        { .code = 0x3, .length = 5 },  { .code = 0x29, .length = 6 },
        { .code = 0x2b, .length = 6 }, { .code = 0x28, .length = 6 },
        { .code = 0x5, .length = 5 },  { .code = 0x17, .length = 6 },
        { .code = 0x4, .length = 5 },  { .code = 0x7, .length = 5 },
        { .code = 0x29, .length = 6 }
    };
    uint8_t buffer[12]        = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    uint8_t expected_result[] = { 0xf1, 0xe3, 0xc2, 0xe5, 0xf2, 0x3a,
                                  0x6b, 0xa0, 0xab, 0x90, 0xf4, 0xff };

    int8_t rs =
      hpack_encoder_pack_encoded_words_to_bytes(encoded_buffer, 15, buffer, 12);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 12; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}

void test_pack_encoded_words_to_bytes_test2(void)
{
    huffman_encoded_word_t encoded_buffer[] = {
        /*no-cache*/
        { .code = 0x2a, .length = 6 }, { .code = 0x7, .length = 5 },
        { .code = 0x16, .length = 6 }, { .code = 0x4, .length = 5 },
        { .code = 0x3, .length = 5 },  { .code = 0x4, .length = 5 },
        { .code = 0x27, .length = 6 }, { .code = 0x5, .length = 5 },
    };
    uint8_t buffer[6] = { 0, 0, 0, 0, 0, 0 };
    // a8eb 1064 9cbf
    uint8_t expected_result[] = { 0xa8, 0xeb, 0x10, 0x64, 0x9c, 0xbf };

    int8_t rs =
      hpack_encoder_pack_encoded_words_to_bytes(encoded_buffer, 8, buffer, 6);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 6; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}

void test_pack_encoded_words_to_bytes_test3(void)
{
    huffman_encoded_word_t encoded_buffer[] = {
        /*custom-value*/
        { .code = 0x4, .length = 5 },  { .code = 0x2d, .length = 6 },
        { .code = 0x8, .length = 5 },  { .code = 0x9, .length = 5 },
        { .code = 0x7, .length = 5 },  { .code = 0x29, .length = 6 },
        { .code = 0x16, .length = 6 }, /*-*/
        { .code = 0x77, .length = 7 }, { .code = 0x3, .length = 5 },
        { .code = 0x28, .length = 6 }, { .code = 0x2d, .length = 6 },
        { .code = 0x5, .length = 5 },
    };
    uint8_t buffer[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    // 25a8 49e9 5bb8 e8b4 bf
    uint8_t expected_result[] = { 0x25, 0xa8, 0x49, 0xe9, 0x5b,
                                  0xb8, 0xe8, 0xb4, 0xbf };

    int8_t rs =
      hpack_encoder_pack_encoded_words_to_bytes(encoded_buffer, 12, buffer, 9);

    TEST_ASSERT_EQUAL(0, rs);
    for (int i = 0; i < 9; i++) {
        TEST_ASSERT_EQUAL(expected_result[i], buffer[i]);
    }
}

void test_encode_huffman_word(void)
{
    uint32_t str_length = 15;
    char *str           = "www.example.com";

    SET_CUSTOM_FAKE_SEQ(
      hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);

    huffman_encoded_word_t encoded_words[str_length];
    uint32_t encoded_word_bit_length =
      hpack_encoder_encode_huffman_word(str, str_length, encoded_words);
    TEST_ASSERT_EQUAL(89, encoded_word_bit_length);
}

void test_encode_huffman_string(void)
{
    char *str = "www.example.com";
    uint8_t encoded_string[30];

    uint8_t *expected_encoded_string = encoded_wwwdotexampledotcom;

    hpack_utils_encoded_integer_size_fake.return_val = 1;
    SET_CUSTOM_FAKE_SEQ(
      hpack_huffman_encode, hpack_huffman_encode_wwwdotexampledotcom_arr, 15);
    int rc = hpack_encoder_encode_huffman_string(str, encoded_string, 30);
    TEST_ASSERT_EQUAL(13, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_string[i], encoded_string[i]);
    }
}

void test_encode_literal_header_field_new_name(void)
{
    /*Test encoding a string*/
    char *name_to_encode  = "custom-key";
    char *value_to_encode = "custom-value";

    hpack_encoded_header_t header;
    header.preamble = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    header.index    = 0;

    /*String huffman compressed*/
    uint8_t expected_string_encoded[] = { 0x40, 0x88, 0x25, 0xa8, 0x49,
                                          0xe9, 0x5b, 0xa9, 0x7d, 0x7f,
                                          0x89, 0x25, 0xa8, 0x49, 0xe9,
                                          0x5b, 0xb8, 0xe8, 0xb4, 0xbf };
    uint8_t expected_bytes            = 20;
    /*Set up encoding function call*/
    int8_t (*hpack_huffman_encode_huffman_header_name_value[22])(
      huffman_encoded_word_t *, uint8_t);
    for (uint8_t i = 0; i < strlen(name_to_encode); i++) {
        hpack_huffman_encode_huffman_header_name_value[i] =
          hpack_huffman_encode_customkey_arr[i];
    }
    for (uint8_t i = strlen(name_to_encode);
         i < strlen(value_to_encode) + strlen(name_to_encode);
         i++) {
        hpack_huffman_encode_huffman_header_name_value[i] =
          hpack_huffman_encode_customvalue_arr[i - strlen(name_to_encode)];
    }
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode,
                        hpack_huffman_encode_huffman_header_name_value,
                        strlen(name_to_encode) + strlen(value_to_encode));

    uint8_t encoded_buffer[expected_bytes];
    memset(encoded_buffer, 0, expected_bytes);
    hpack_utils_encoded_integer_size_fake.return_val = 1;

    int8_t rc = hpack_encoder_encode_header(
      &header, name_to_encode, value_to_encode, encoded_buffer, expected_bytes);
    TEST_ASSERT_EQUAL(expected_bytes, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_string_encoded[i], encoded_buffer[i]);
    }
}

void test_encode_literal_header_field_new_name_error(void)
{
    /*Test border condition*/
    char name_to_encode[2 * HTTP2_STREAM_BUF_SIZE];
    char value_to_encode[2 * HTTP2_STREAM_BUF_SIZE];
    uint8_t encoded_string[HTTP2_STREAM_BUF_SIZE];
    hpack_encoded_header_t encoded_header;

    memset(encoded_string, 0, HTTP2_STREAM_BUF_SIZE);
    memset(name_to_encode, 0, 2 * HTTP2_STREAM_BUF_SIZE);
    memset(value_to_encode, 0, 2 * HTTP2_STREAM_BUF_SIZE);

    uint32_t hpack_utils_encoded_integer_size_fake_seq[] = { 1, 2, 2, 1, 2 };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size,
                   hpack_utils_encoded_integer_size_fake_seq,
                   5);

    hpack_huffman_encode_fake.custom_fake = hpack_huffman_encode_return_w;
    for (int i = 0; i < 2 * HTTP2_STREAM_BUF_SIZE - 1; i++) {
        name_to_encode[i]  = 'w';
        value_to_encode[i] = 'w';
    }
    encoded_header.index    = 0;
    encoded_header.preamble = LITERAL_HEADER_FIELD_NEVER_INDEXED;
    int rc                  = hpack_encoder_encode_header(&encoded_header,
                                         name_to_encode,
                                         value_to_encode,
                                         encoded_string,
                                         HTTP2_STREAM_BUF_SIZE);
    TEST_ASSERT_EQUAL(-1, rc);
    char name_to_encode2[10];
    for (int i = 0; i < 10; i++) {
        name_to_encode2[i] = 'w';
    }
    rc = hpack_encoder_encode_header(&encoded_header,
                                     name_to_encode2,
                                     value_to_encode,
                                     encoded_string,
                                     HTTP2_STREAM_BUF_SIZE);
    TEST_ASSERT_EQUAL(-1, rc);
}

void test_hpack_encoder_encode_test1(void)
{
    /*Test indexed header field*/
    int expected_encoding[] = { 0x82 };

    http_header_t headers[] = {
        { .name = ":method", .value = "GET" },
    };
    header_list_count_fake.return_val = 1;
    header_list_all_fake.return_val   = headers;
    header_list_t header_list;

    uint8_t encoded_buffer[] = { 0 };

    hpack_dynamic_table_t dynamic_table;
    memset(dynamic_table.buffer, 0, HPACK_MAX_DYNAMIC_TABLE_SIZE);
    dynamic_table.max_size    = HPACK_MAX_DYNAMIC_TABLE_SIZE;
    dynamic_table.actual_size = 0;
    dynamic_table.n_entries   = 0;
    dynamic_table.first       = 0;
    dynamic_table.next        = 0;

    hpack_tables_find_index_fake.custom_fake =
      hpack_tables_find_index_fake_method_get;
    hpack_utils_find_prefix_size_fake.return_val     = 7;
    hpack_utils_encoded_integer_size_fake.return_val = 1;

    int rc =
      hpack_encoder_encode(&dynamic_table, &header_list, encoded_buffer, 1);
    TEST_ASSERT_EQUAL(1, rc);
    TEST_ASSERT_EQUAL(expected_encoding[0], encoded_buffer[0]);
}

void test_hpack_encoder_encode_test2(void)
{
    /*Test new name*/
    uint8_t encoded_buffer[64];

    http_header_t headers[] = {
        { .name = "name", .value = "val" },
    };
    header_list_count_fake.return_val = 1;
    header_list_all_fake.return_val   = headers;
    header_list_t header_list;

    hpack_dynamic_table_t dynamic_table;
    memset(dynamic_table.buffer, 0, HPACK_MAX_DYNAMIC_TABLE_SIZE);
    dynamic_table.max_size           = HPACK_MAX_DYNAMIC_TABLE_SIZE;
    dynamic_table.actual_size        = 0;
    dynamic_table.n_entries          = 0;
    dynamic_table.first              = 0;
    dynamic_table.next               = 0;
    uint8_t expected_encoded_bytes[] = {
        16, // LITERAL_HEADER_FIELD_NEVER_INDEXED, index=0
        4,  // name_length
        (uint8_t)'n',
        (uint8_t)'a',
        (uint8_t)'m',
        (uint8_t)'e',
        3, // value_length
        (uint8_t)'v',
        (uint8_t)'a',
        (uint8_t)'l'
    };

    uint32_t hpack_utils_encoded_integer_size_fake_seq[] = {
        2 * HTTP2_STREAM_BUF_SIZE, 1, 2 * HTTP2_STREAM_BUF_SIZE, 1
    };

    SET_RETURN_SEQ(hpack_utils_encoded_integer_size,
                   hpack_utils_encoded_integer_size_fake_seq,
                   4);

    hpack_utils_find_prefix_size_fake.return_val         = 4;
    hpack_tables_find_index_fake.return_val              = -1;
    hpack_tables_find_index_name_fake.return_val         = -1;
    hpack_tables_dynamic_table_add_entry_fake.return_val = -1;

    int rc =
      hpack_encoder_encode(&dynamic_table, &header_list, encoded_buffer, 64);

    TEST_ASSERT_EQUAL(10, rc);

#if HPACK_INCLUDE_DYNAMIC_TABLE
    TEST_ASSERT_EQUAL_STRING(
      "name", hpack_tables_dynamic_table_add_entry_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(
      "val", hpack_tables_dynamic_table_add_entry_fake.arg2_val);
#endif
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_bytes[i], encoded_buffer[i]);
    }
}

void test_hpack_encoder_encode_test3(void)
{
    /*Test new name*/
    http_header_t headers[] = {
        { .name = ":authority", .value = "val" },
    };
    header_list_count_fake.return_val = 1;
    header_list_all_fake.return_val   = headers;
    header_list_t header_list;

    uint8_t encoded_buffer[5];
    memset(encoded_buffer, 0, 5);
    uint8_t expected_encoded_bytes[] = {
        17, // LITERAL_HEADER_FIELD_NEVER_INDEXED, index=1
        3,  // value_length
        (uint8_t)'v',
        (uint8_t)'a',
        (uint8_t)'l'
    };

    hpack_utils_find_prefix_size_fake.return_val = 4;

    // initialize dynamic table
    hpack_dynamic_table_t dynamic_table;
    memset(dynamic_table.buffer, 0, HPACK_MAX_DYNAMIC_TABLE_SIZE);
    dynamic_table.max_size    = HPACK_MAX_DYNAMIC_TABLE_SIZE;
    dynamic_table.actual_size = 0;
    dynamic_table.n_entries   = 0;
    dynamic_table.first       = 0;
    dynamic_table.next        = 0;

    hpack_utils_find_prefix_size_fake.return_val         = 4;
    uint32_t hpack_utils_encoded_integer_size_fake_seq[] = {
        1, 2 * HTTP2_STREAM_BUF_SIZE, 1
    };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size,
                   hpack_utils_encoded_integer_size_fake_seq,
                   3);
    hpack_tables_find_index_fake.return_val              = -1;
    hpack_tables_find_index_name_fake.return_val         = 1;
    hpack_tables_dynamic_table_add_entry_fake.return_val = -1;

    int rc =
      hpack_encoder_encode(&dynamic_table, &header_list, encoded_buffer, 5);

    TEST_ASSERT_EQUAL(5, rc);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    TEST_ASSERT_EQUAL_STRING(
      ":authority", hpack_tables_dynamic_table_add_entry_fake.arg1_val);
    TEST_ASSERT_EQUAL_STRING(
      "val", hpack_tables_dynamic_table_add_entry_fake.arg2_val);
#endif
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_bytes[i], encoded_buffer[i]);
    }
}

void test_encode_indexed_header_field(void)
{
    int expected_encoding[]                = { 130, 134, 132 };
    uint8_t encoded_buffer[]               = { 0, 0, 0 };
    int fake_return_seq_hpack_find_index[] = { 2, 6, 4 };

    http_header_t headers[] = {
        { .name = ":method", .value = "GET" },
        { .name = ":scheme", .value = "http" },
        { .name = ":path", .value = "/" },
    };
    header_list_count_fake.return_val = 3;
    header_list_all_fake.return_val   = headers;
    header_list_t header_list;

    // initialize dynamic table
    hpack_dynamic_table_t dynamic_table;
    memset(dynamic_table.buffer, 0, HPACK_MAX_DYNAMIC_TABLE_SIZE);
    dynamic_table.max_size    = HPACK_MAX_DYNAMIC_TABLE_SIZE;
    dynamic_table.actual_size = 0;
    dynamic_table.n_entries   = 0;
    dynamic_table.first       = 0;
    dynamic_table.next        = 0;

    SET_RETURN_SEQ(
      hpack_tables_find_index, fake_return_seq_hpack_find_index, 3);
    hpack_utils_find_prefix_size_fake.return_val     = 7;
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    int rc =
      hpack_encoder_encode(&dynamic_table, &header_list, encoded_buffer, 3);
    TEST_ASSERT_EQUAL(3, rc);
    for (int i = 0; i < 3; i++) {
        TEST_ASSERT_EQUAL(expected_encoding[i], encoded_buffer[i]);
    }
}

void test_encode_literal_header_field_indexed_name(void)
{
    /*Test with no compression*/
    uint8_t expected_encoded_buffer[] = { 0x44, 0x0c, 0x2f, 0x73, 0x61,
                                          0x6d, 0x70, 0x6c, 0x65, 0x2f,
                                          0x70, 0x61, 0x74, 0x68 };
    int expected_len                  = 14;
    uint8_t encoded_buffer[14];
    hpack_encoded_header_t header;
    header.preamble = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    header.index    = 4;

    memset(encoded_buffer, 0, expected_len);
    char value_string[]                  = "/sample/path\0";
    uint32_t encoded_integer_fake_seq[3] = { 1, HTTP2_STREAM_BUF_SIZE, 1 };
    SET_RETURN_SEQ(
      hpack_utils_encoded_integer_size, encoded_integer_fake_seq, 3);
    hpack_huffman_encode_fake.custom_fake =
      hpack_huffman_encode_return_infinity; // SETUP COMPRESSION
    hpack_utils_find_prefix_size_fake.return_val = 6;
    int rc                                       = hpack_encoder_encode_header(
      &header, ":path", value_string, encoded_buffer, 14);
    TEST_ASSERT_EQUAL(expected_len, rc);
    for (int i = 0; i < expected_len; i++) {
        TEST_ASSERT_EQUAL(expected_encoded_buffer[i], encoded_buffer[i]);
    }
    setUp();
    /*Test with compression*/
    char value_string_huffman[] = "www.example.com";
    memset(encoded_buffer, 0, 14);
    hpack_utils_encoded_integer_size_fake.return_val = 1;
    SET_CUSTOM_FAKE_SEQ(hpack_huffman_encode,
                        hpack_huffman_encode_wwwdotexampledotcom_arr,
                        strlen(value_string_huffman));
    rc = hpack_encoder_encode_header(
      &header, ":path", value_string_huffman, encoded_buffer, 14);
    TEST_ASSERT_EQUAL(14, rc);
    for (int i = 0; i < rc - 1; i++) {
        TEST_ASSERT_EQUAL(encoded_wwwdotexampledotcom[i],
                          encoded_buffer[i + 1]);
    }
}

void test_encode_literal_header_field_indexed_name_error(void)
{
    /*Test border condition*/
    char value_to_encode[2 * HPACK_MAXIMUM_INTEGER];

    memset(value_to_encode, 'w', 2 * HPACK_MAXIMUM_INTEGER);

    uint8_t encoded_string[HPACK_MAXIMUM_INTEGER];

    hpack_encoded_header_t header;
    header.preamble = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    header.index    = 4;

    memset(encoded_string, 0, HPACK_MAXIMUM_INTEGER);

    uint32_t hpack_utils_encoded_integer_size_fake_seq[] = { 1, 1, 2, 2, 1, 2 };
    SET_RETURN_SEQ(hpack_utils_encoded_integer_size,
                   hpack_utils_encoded_integer_size_fake_seq,
                   6);

    hpack_huffman_encode_fake.custom_fake = hpack_huffman_encode_return_w;
    int rc                                = hpack_encoder_encode_header(
      &header, ":path", value_to_encode, encoded_string, HPACK_MAXIMUM_INTEGER);
    TEST_ASSERT_EQUAL(-1, rc);
}

#if HPACK_INCLUDE_DYNAMIC_TABLE
void test_encode_dynamic_size_update(void)
{
    uint8_t expected_encoded[] = { 0x3f, 0x8f, 0x2 };
    uint8_t encoded_buffer[3];

    memset(encoded_buffer, 0, 3);
    uint32_t new_max_size                            = 302;
    hpack_utils_encoded_integer_size_fake.return_val = 3;
    hpack_dynamic_table_t dynamic_table;
    memset(dynamic_table.buffer, 0, 4092);
    dynamic_table.max_size    = 4092;
    dynamic_table.actual_size = 0;
    dynamic_table.n_entries   = 0;
    dynamic_table.first       = 0;
    dynamic_table.next        = 0;
    int rc                    = hpack_encoder_encode_dynamic_size_update(
      &dynamic_table, new_max_size, encoded_buffer);
    TEST_ASSERT_EQUAL(3, rc);
    for (int i = 0; i < rc; i++) {
        TEST_ASSERT_EQUAL(expected_encoded[i], encoded_buffer[i]);
    }
}
#endif

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_hpack_encoder_encode_test1);
    UNIT_TEST(test_hpack_encoder_encode_test2);
    UNIT_TEST(test_hpack_encoder_encode_test3);
    UNIT_TEST(test_encode_integer);

    UNIT_TEST(test_pack_encoded_words_to_bytes_test1);
    UNIT_TEST(test_pack_encoded_words_to_bytes_test2);
    UNIT_TEST(test_pack_encoded_words_to_bytes_test3);
    UNIT_TEST(test_encode_huffman_word);
    UNIT_TEST(test_encode_huffman_string);

    UNIT_TEST(test_encode_non_huffman_string);
    UNIT_TEST(test_encode_literal_header_field_new_name);
    UNIT_TEST(test_encode_literal_header_field_new_name_error);
    UNIT_TEST(test_encode_literal_header_field_indexed_name);
    UNIT_TEST(test_encode_literal_header_field_indexed_name_error);
    UNIT_TEST(test_encode_indexed_header_field);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    UNIT_TEST(test_encode_dynamic_size_update);
#endif
    return UNIT_TESTS_END();
}
