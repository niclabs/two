#include "unit.h"
#include "fff.h"
#include "logging.h"
#include "hpack_utils.h"
#include "table.h"

extern uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer);

DEFINE_FFF_GLOBALS;
/*
FAKE_VALUE_FUNC(int8_t, hpack_huffman_encode, huffman_encoded_word_t *, uint8_t);
FAKE_VALUE_FUNC(int8_t, hpack_huffman_decode, huffman_encoded_word_t *, uint8_t *);
FAKE_VALUE_FUNC(int,  headers_add, headers_t * , const char * , const char * );
*/

void setUp(void)
{
    /* Register resets */
    //FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_hpack_utils_read_bits_from_bytes(void)
{
    uint8_t buffer[] = { 0xD1, 0xC5, 0x6E };
    uint32_t code = 0;

    //Test if it reads 1 byte correctly
    code = hpack_utils_read_bits_from_bytes(0, 8, buffer);
    TEST_ASSERT_EQUAL(0xD1, code);

    code = hpack_utils_read_bits_from_bytes(8, 8, buffer);
    TEST_ASSERT_EQUAL(0xC5, code);

    code = hpack_utils_read_bits_from_bytes(16, 8, buffer);
    TEST_ASSERT_EQUAL(0x6E, code);

    //Test reading different number of bits
    code = hpack_utils_read_bits_from_bytes(0, 1, buffer);
    TEST_ASSERT_EQUAL(0x1, code);

    code = hpack_utils_read_bits_from_bytes(0, 3, buffer);
    TEST_ASSERT_EQUAL(0x6, code);

    code = hpack_utils_read_bits_from_bytes(0, 5, buffer);
    TEST_ASSERT_EQUAL(0x1A, code);

    //Test reading between bytes
    code = hpack_utils_read_bits_from_bytes(4, 8, buffer);
    TEST_ASSERT_EQUAL(0x1C, code);

    code = hpack_utils_read_bits_from_bytes(12, 8, buffer);
    TEST_ASSERT_EQUAL(0x56, code);

    code = hpack_utils_read_bits_from_bytes(4, 16, buffer);
    TEST_ASSERT_EQUAL(0x1C56, code);

}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_hpack_utils_read_bits_from_bytes);

    return UNIT_TESTS_END();
}
