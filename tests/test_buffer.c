//
// Created by maite on 07-05-19.
//

#include <errno.h>

#include "buffer.h"
#include "fff.h"
#include "unit.h"

DEFINE_FFF_GLOBALS;
/*FAKE_VALUE_FUNC(int, u24, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, u31, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, u32, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, u16, uint16_t, uint8_t*);

FAKE_VALUE_FUNC(uint32_t, get_u32, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, get_u31, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, get_u32_24, uint8_t*);
FAKE_VALUE_FUNC(uint16_t, get_u16, uint8_t*);
FAKE_VALUE_FUNC(int, append_byte_arrays,uint8_t*, uint8_t*, int, int);
*/
/* List of fakes used by this unit tester */
/*#define FFF_FAKES_LIST(FAKE)        \
    FAKE(u24)   \
    FAKE(u31)   \
    FAKE(u32)      \
    FAKE(u16)      \
    FAKE(get_u32)           \
    FAKE(get_u31)        \
    FAKE(get_u32_24)        \
    FAKE(get_u16)           \
    FAKE(append_byte_arrays)
*/
void setUp(void)
{
    /* Register resets */
    // FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_buffer_put_u32_0(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 0);
    TEST_ASSERT_EQUAL(0, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(0, byte_array[2]);
    TEST_ASSERT_EQUAL(0, byte_array[3]);
}

void test_buffer_put_u32_12(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 12);
    TEST_ASSERT_EQUAL(12, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(0, byte_array[2]);
    TEST_ASSERT_EQUAL(12, byte_array[3]);
}

void test_buffer_put_u32_300(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 300);
    TEST_ASSERT_EQUAL(300, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(1, byte_array[2]);
    TEST_ASSERT_EQUAL(44, byte_array[3]);
}

void test_buffer_put_u32_200000(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 200000);
    TEST_ASSERT_EQUAL(200000, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(3, byte_array[1]);
    TEST_ASSERT_EQUAL(13, byte_array[2]);
    TEST_ASSERT_EQUAL(64, byte_array[3]);
}

void test_buffer_put_u32_400000000(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 400000000);
    TEST_ASSERT_EQUAL(400000000, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(23, byte_array[0]);
    TEST_ASSERT_EQUAL(215, byte_array[1]);
    TEST_ASSERT_EQUAL(132, byte_array[2]);
    TEST_ASSERT_EQUAL(0, byte_array[3]);
}

void test_buffer_put_u32_4294967295(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 4294967295);
    TEST_ASSERT_EQUAL(4294967295, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(255, byte_array[0]);
    TEST_ASSERT_EQUAL(255, byte_array[1]);
    TEST_ASSERT_EQUAL(255, byte_array[2]);
    TEST_ASSERT_EQUAL(255, byte_array[3]);
}

void test_buffer_put_u32_2147483648(void)
{
    uint8_t byte_array[4];
    buffer_put_u32(byte_array, 2147483648);
    TEST_ASSERT_EQUAL(2147483648, buffer_get_u32(byte_array));
    TEST_ASSERT_EQUAL(128, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(0, byte_array[2]);
    TEST_ASSERT_EQUAL(0, byte_array[3]);
}

void test_buffer_put_u31_2147483648(void)
{
    uint8_t byte_array[4];
    buffer_put_u31(byte_array, 2147483648);
    TEST_ASSERT_EQUAL(0, buffer_get_u31(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(0, byte_array[2]);
    TEST_ASSERT_EQUAL(0, byte_array[3]);
}

void test_buffer_put_u31_256(void)
{
    uint8_t byte_array[4];
    buffer_put_u31(byte_array, 256);
    TEST_ASSERT_EQUAL(256, buffer_get_u31(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(1, byte_array[2]);
    TEST_ASSERT_EQUAL(0, byte_array[3]);
}

void test_buffer_put_u31_2147483647(void)
{
    uint8_t byte_array[4];
    buffer_put_u31(byte_array, 2147483647);
    TEST_ASSERT_EQUAL(2147483647, buffer_get_u31(byte_array));
    TEST_ASSERT_EQUAL(127, byte_array[0]);
    TEST_ASSERT_EQUAL(255, byte_array[1]);
    TEST_ASSERT_EQUAL(255, byte_array[2]);
    TEST_ASSERT_EQUAL(255, byte_array[3]);
}

void test_buffer_put_u24_16777215(void)
{
    uint8_t byte_array[3];
    buffer_put_u24(byte_array, 16777215);
    TEST_ASSERT_EQUAL(16777215, buffer_get_u24(byte_array));
    TEST_ASSERT_EQUAL(255, byte_array[0]);
    TEST_ASSERT_EQUAL(255, byte_array[1]);
    TEST_ASSERT_EQUAL(255, byte_array[2]);
}

void test_buffer_put_u24_16777216(void)
{
    uint8_t byte_array[3];
    buffer_put_u24(byte_array, 16777216);
    TEST_ASSERT_EQUAL(0, buffer_get_u24(byte_array));
    TEST_ASSERT_EQUAL(0, byte_array[0]);
    TEST_ASSERT_EQUAL(0, byte_array[1]);
    TEST_ASSERT_EQUAL(0, byte_array[2]);
}

void test_buffer_put_u16_65535(void)
{
    uint8_t byte_array[2];
    buffer_put_u16(byte_array, 65535);
    TEST_ASSERT_EQUAL(65535, buffer_get_u16(byte_array));
    TEST_ASSERT_EQUAL(255, byte_array[0]);
    TEST_ASSERT_EQUAL(255, byte_array[1]);
}

void test_get_u32_0(void)
{
    uint8_t bytes[4] = { 0, 0, 0, 0 };
    TEST_ASSERT_EQUAL(0, buffer_get_u32(bytes));
}
void test_get_u32_12(void)
{
    uint8_t bytes[4] = { 0, 0, 0, 12 };
    TEST_ASSERT_EQUAL(12, buffer_get_u32(bytes));
}
void test_get_u32_300(void)
{
    uint8_t bytes[4] = { 0, 0, 1, 44 };
    TEST_ASSERT_EQUAL(300, buffer_get_u32(bytes));
}
void test_get_u32_200000(void)
{
    uint8_t bytes[4] = { 0, 3, 13, 64 };
    TEST_ASSERT_EQUAL(200000, buffer_get_u32(bytes));
}

void test_get_u32_400000000(void)
{
    uint8_t bytes[4] = { 23, 215, 132, 0 };
    TEST_ASSERT_EQUAL(400000000, buffer_get_u32(bytes));
}

void test_get_u32_4294967295(void)
{
    uint8_t bytes[4] = { 255, 255, 255, 255 };
    TEST_ASSERT_EQUAL(4294967295, buffer_get_u32(bytes));
}

void test_get_u32_2147483648(void)
{
    uint8_t bytes[4] = { 128, 0, 0, 0 };
    TEST_ASSERT_EQUAL(2147483648, buffer_get_u32(bytes));
}

void test_get_u31_2147483648(void)
{
    uint8_t bytes[4] = { 128, 0, 0, 0 };
    TEST_ASSERT_EQUAL(0, buffer_get_u31(bytes));
}
void test_get_u31_2147483647(void)
{
    uint8_t bytes[4] = { 255, 255, 255, 255 };
    TEST_ASSERT_EQUAL(2147483647, buffer_get_u31(bytes));
}

void test_get_u24_0(void)
{
    uint8_t bytes[3] = { 0, 0, 0 };
    TEST_ASSERT_EQUAL(0, buffer_get_u24(bytes));
}
void test_get_u24_16777215(void)
{
    uint8_t bytes[3] = { 255, 255, 255 };
    TEST_ASSERT_EQUAL(16777215, buffer_get_u24(bytes));
}

void test_get_u16_0(void)
{
    uint8_t bytes[2] = { 0, 0 };
    TEST_ASSERT_EQUAL(0, buffer_get_u16(bytes));
}

void test_get_u16_65535(void)
{
    uint8_t bytes[2] = { 255, 255 };
    TEST_ASSERT_EQUAL(65535, buffer_get_u16(bytes));
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_buffer_put_u32_0);
    UNIT_TEST(test_buffer_put_u32_12);
    UNIT_TEST(test_buffer_put_u32_300);
    UNIT_TEST(test_buffer_put_u32_200000);
    UNIT_TEST(test_buffer_put_u32_400000000);
    UNIT_TEST(test_buffer_put_u32_4294967295);
    UNIT_TEST(test_buffer_put_u32_2147483648);

    UNIT_TEST(test_buffer_put_u31_2147483648);
    UNIT_TEST(test_buffer_put_u31_2147483647);
    UNIT_TEST(test_buffer_put_u31_256);

    UNIT_TEST(test_buffer_put_u24_16777216);
    UNIT_TEST(test_buffer_put_u24_16777215);

    UNIT_TEST(test_buffer_put_u16_65535);

    UNIT_TEST(test_get_u32_0);
    UNIT_TEST(test_get_u32_12);
    UNIT_TEST(test_get_u32_300);
    UNIT_TEST(test_get_u32_200000);
    UNIT_TEST(test_get_u32_400000000);
    UNIT_TEST(test_get_u32_4294967295);
    UNIT_TEST(test_get_u32_2147483648);

    UNIT_TEST(test_get_u31_2147483648);
    UNIT_TEST(test_get_u31_2147483647);

    UNIT_TEST(test_get_u24_0);
    UNIT_TEST(test_get_u24_16777215);

    UNIT_TEST(test_get_u16_0);
    UNIT_TEST(test_get_u16_65535);

    return UNIT_TESTS_END();
}
