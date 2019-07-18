//
// Created by maite on 07-05-19.
//

#include <errno.h>

#include "unit.h"
#include "utils.h"
#include "fff.h"


DEFINE_FFF_GLOBALS;
/*FAKE_VALUE_FUNC(int, uint32_24_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_31_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint32_to_byte_array, uint32_t, uint8_t*);
FAKE_VALUE_FUNC(int, uint16_to_byte_array, uint16_t, uint8_t*);

FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_31, uint8_t*);
FAKE_VALUE_FUNC(uint32_t, bytes_to_uint32_24, uint8_t*);
FAKE_VALUE_FUNC(uint16_t, bytes_to_uint16, uint8_t*);
FAKE_VALUE_FUNC(int, append_byte_arrays,uint8_t*, uint8_t*, int, int);
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
    FAKE(append_byte_arrays)
*/
void setUp(void) {
    /* Register resets */
    //FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}


void test_uint32_to_byte_array_0(void){
    uint32_t test_0 = 0;
    uint8_t byte_array_expected[4] = {0,0,0,0};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_0,byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0],byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1],byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2],byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3],byte_array_result[3]);
}

void test_uint32_to_byte_array_12(void){
    uint32_t test_12 = 12;
    uint8_t byte_array_expected[4] = {0,0,0,12};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_12,byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0],byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1],byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2],byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3],byte_array_result[3]);
}

void test_uint32_to_byte_array_300(void){
    uint32_t test_300 = 300;
    uint8_t byte_array_expected[4] = {0,0,1,44};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_300,byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0],byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1],byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2],byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3],byte_array_result[3]);
}

void test_uint32_to_byte_array_200000(void){
    uint32_t test_200000 = 200000;
    uint8_t byte_array_expected[4] = {0,3,13,64};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_200000,byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0],byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1],byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2],byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3],byte_array_result[3]);
}

void test_uint32_to_byte_array_400000000(void) {
    uint32_t test_400000000 = 400000000;
    uint8_t byte_array_expected[4] = {23, 215, 132, 0};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_400000000, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3], byte_array_result[3]);

}


void test_uint32_to_byte_array_4294967295(void) {
    uint32_t test_4294967295 = 4294967295;
    uint8_t byte_array_expected[4] = {255, 255, 255, 255};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_4294967295, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3], byte_array_result[3]);
}
void test_uint32_to_byte_array_2147483648(void) {
    uint32_t test_2147483648 = 2147483648;
    uint8_t byte_array_expected[4] = {128, 0, 0, 0};
    uint8_t byte_array_result[4];
    uint32_to_byte_array(test_2147483648, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3], byte_array_result[3]);
}

void test_uint32_31_to_byte_array_2147483648(void) {
    uint32_t test_2147483648 = 2147483648;
    uint8_t byte_array_expected[4] = {0, 0, 0, 0};
    uint8_t byte_array_result[4];
    uint32_31_to_byte_array(test_2147483648, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3], byte_array_result[3]);
}

void test_uint32_31_to_byte_array_2147483647(void) {
    uint32_t test_2147483647 = 2147483647;
    uint8_t byte_array_expected[4] = {127, 255, 255, 255};
    uint8_t byte_array_result[4];
    uint32_31_to_byte_array(test_2147483647, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
    TEST_ASSERT_EQUAL(byte_array_expected[3], byte_array_result[3]);
}

void test_uint32_24_to_byte_array_16777215(void) {
    uint32_t test_16777215 = 16777215;
    uint8_t byte_array_expected[3] = {255, 255, 255};
    uint8_t byte_array_result[3];
    uint32_24_to_byte_array(test_16777215, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
}

void test_uint32_24_to_byte_array_16777216(void) {
    uint32_t test_16777216 = 16777216;
    uint8_t byte_array_expected[3] = { 0, 0, 0};
    uint8_t byte_array_result[3];
    uint32_24_to_byte_array(test_16777216, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
    TEST_ASSERT_EQUAL(byte_array_expected[2], byte_array_result[2]);
}

void test_uint16_to_byte_array_65535(void) {
    uint16_t test_65535 = 65535;
    uint8_t byte_array_expected[2] = {255, 255};
    uint8_t byte_array_result[2];
    uint16_to_byte_array(test_65535, byte_array_result);
    TEST_ASSERT_EQUAL(byte_array_expected[0], byte_array_result[0]);
    TEST_ASSERT_EQUAL(byte_array_expected[1], byte_array_result[1]);
}


void test_bytes_to_uint32_0(void){
    uint8_t bytes[4] = {0,0,0,0};
    uint32_t expected = 0;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_12(void){
    uint8_t bytes[4] = {0,0,0,12};
    uint32_t expected = 12;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_300(void){
    uint8_t bytes[4] = {0,0,1,44};
    uint32_t expected = 300;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_200000(void){
    uint8_t bytes[4] = {0,3,13,64};
    uint32_t expected = 200000;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_400000000(void){
    uint8_t bytes[4] = {23, 215, 132, 0};
    uint32_t expected = 400000000;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_4294967295(void){
    uint8_t bytes[4] = {255, 255, 255, 255};
    uint32_t expected = 4294967295;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_2147483648(void){
    uint8_t bytes[4] = {128, 0, 0, 0};
    uint32_t expected = 2147483648;
    uint32_t result = bytes_to_uint32(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_31_2147483648(void){
    uint8_t bytes[4] = {128, 0, 0, 0};
    uint32_t expected = 0;
    uint32_t result = bytes_to_uint32_31(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_31_2147483647(void){
    uint8_t bytes[4] = {255, 255, 255, 255};
    uint32_t expected = 2147483647;
    uint32_t result = bytes_to_uint32_31(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}

void test_bytes_to_uint32_24_0(void){
    uint8_t bytes[3] = {0, 0, 0};
    uint32_t expected = 0;
    uint32_t result = bytes_to_uint32_24(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_bytes_to_uint32_24_16777215(void){
    uint8_t bytes[3] = {255, 255, 255};
    uint32_t expected = 16777215;
    uint32_t result = bytes_to_uint32_24(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}

void test_bytes_to_uint16_0(void){
    uint8_t bytes[2] = {0, 0};
    uint32_t expected = 0;
    uint32_t result = bytes_to_uint16(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}

void test_bytes_to_uint16_65535(void){
    uint8_t bytes[2] = {255, 255};
    uint32_t expected = 65535;
    uint32_t result = bytes_to_uint16(bytes);
    TEST_ASSERT_EQUAL(expected,result);
}
void test_append_byte_arrays(void){
    uint8_t bytes_1[2] = {255, 255};
    uint8_t bytes_2[3] = {128, 0, 1};
    uint8_t result[5];
    int total_length = append_byte_arrays(result, bytes_1, bytes_2, 2, 3);


    TEST_ASSERT_EQUAL(5,total_length);

    TEST_ASSERT_EQUAL(bytes_1[0],result[0]);
    TEST_ASSERT_EQUAL(bytes_1[1],result[1]);
    TEST_ASSERT_EQUAL(bytes_2[0],result[2]);
    TEST_ASSERT_EQUAL(bytes_2[1],result[3]);
    TEST_ASSERT_EQUAL(bytes_2[2],result[4]);


}

void test_buffer_copy(void){

    uint8_t orig[] = {
            1,2,3,4,5,6,7,8,9,0
    };
    uint8_t dest[10];

    int rc = buffer_copy(dest,orig,10);
    TEST_ASSERT_EQUAL(10, rc);

    for(int i = 0; i < rc; i++){
        TEST_ASSERT_EQUAL(orig[i], dest[i]);
    }


}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_uint32_to_byte_array_0);
    UNIT_TEST(test_uint32_to_byte_array_12);
    UNIT_TEST(test_uint32_to_byte_array_300);
    UNIT_TEST(test_uint32_to_byte_array_200000);
    UNIT_TEST(test_uint32_to_byte_array_400000000);
    UNIT_TEST(test_uint32_to_byte_array_4294967295);
    UNIT_TEST(test_uint32_to_byte_array_2147483648);

    UNIT_TEST(test_uint32_31_to_byte_array_2147483648);
    UNIT_TEST(test_uint32_31_to_byte_array_2147483647);

    UNIT_TEST(test_uint32_24_to_byte_array_16777216);
    UNIT_TEST(test_uint32_24_to_byte_array_16777215);


    UNIT_TEST(test_uint16_to_byte_array_65535);


    UNIT_TEST(test_bytes_to_uint32_0);
    UNIT_TEST(test_bytes_to_uint32_12);
    UNIT_TEST(test_bytes_to_uint32_300);
    UNIT_TEST(test_bytes_to_uint32_200000);
    UNIT_TEST(test_bytes_to_uint32_400000000);
    UNIT_TEST(test_bytes_to_uint32_4294967295);
    UNIT_TEST(test_bytes_to_uint32_2147483648);

    UNIT_TEST(test_bytes_to_uint32_31_2147483648);
    UNIT_TEST(test_bytes_to_uint32_31_2147483647);

    UNIT_TEST(test_bytes_to_uint32_24_0);
    UNIT_TEST(test_bytes_to_uint32_24_16777215);

    UNIT_TEST(test_bytes_to_uint16_0);
    UNIT_TEST(test_bytes_to_uint16_65535);

    UNIT_TEST(test_append_byte_arrays);

    UNIT_TEST(test_buffer_copy);

    return UNIT_TESTS_END();
}
