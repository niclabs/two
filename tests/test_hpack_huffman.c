//
// Created by gabriel on 19-07-19.
//

#include "unit.h"
#include "fff.h"
#include "hpack_huffman.h"

extern const huffman_tree_t huffman_tree;
DEFINE_FFF_GLOBALS;


void setUp(void) {
    /* Register resets */
    /*FFF_FAKES_LIST(RESET_FAKE);*/

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_hpack_huffman_encode_random(void){
    int8_t rs = -1;
    huffman_encoded_word_t result;
    rs = hpack_huffman_encode(&huffman_tree, &result, 0);
    /*Test first value*/
    TEST_ASSERT_EQUAL(0x1ff8, result.code);
    TEST_ASSERT_EQUAL(13, result.length);

    /*Test 10 random values*/

    rs = hpack_huffman_encode(&huffman_tree, &result, 37);
    TEST_ASSERT_EQUAL(0x15, result.code);
    TEST_ASSERT_EQUAL(6, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 46);
    TEST_ASSERT_EQUAL(0x17, result.code);
    TEST_ASSERT_EQUAL(6, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 75);
    TEST_ASSERT_EQUAL(0x66, result.code);
    TEST_ASSERT_EQUAL(7, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 88);
    TEST_ASSERT_EQUAL(0xfc, result.code);
    TEST_ASSERT_EQUAL(8, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 181);
    TEST_ASSERT_EQUAL(0x3fffe1, result.code);
    TEST_ASSERT_EQUAL(22, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 182);
    TEST_ASSERT_EQUAL(0x7fffee, result.code);
    TEST_ASSERT_EQUAL(23, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 210);
    TEST_ASSERT_EQUAL(0x3ffffe6, result.code);
    TEST_ASSERT_EQUAL(26, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 213);
    TEST_ASSERT_EQUAL(0x3ffffe7, result.code);
    TEST_ASSERT_EQUAL(26, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    rs = hpack_huffman_encode(&huffman_tree, &result, 251);
    TEST_ASSERT_EQUAL(0x7ffffed, result.code);
    TEST_ASSERT_EQUAL(27, result.length);
    TEST_ASSERT_EQUAL(0, rs);

    /*Test last value*/

    /*rs = hpack_huffman_encode(&huffman_tree, &result, 256);
    TEST_ASSERT_EQUAL(0x3fffffff, result.code);
    TEST_ASSERT_EQUAL(30, result.length);
    TEST_ASSERT_EQUAL(0, rs);*/

}

/*Test uniqueness of codes*/
void test_hpack_huffman_encode_unique(void) {
    uint32_t codes[HUFFMAN_TABLE_SIZE];
    int8_t rs = -1;
    huffman_encoded_word_t result;

    for(int i = 0; i < HUFFMAN_TABLE_SIZE; i++){
        rs = hpack_huffman_encode(&huffman_tree, &result, i);
        TEST_ASSERT_EQUAL(0, rs);
        codes[i] = result.code;
        for(int j = i - 1; j >= 0; j--) {
            TEST_ASSERT_FALSE(codes[i]==codes[j]);
        }
    }
}

/*Test to check if encode return the same value twice*/
void test_hpack_huffman_encode_twice(void) {
    int8_t rs = -1;
    (void)rs;
    huffman_encoded_word_t result;
    huffman_encoded_word_t result2;

    for(int i = 0; i < HUFFMAN_TABLE_SIZE; i++){
        rs = hpack_huffman_encode(&huffman_tree, &result, i);
        rs = hpack_huffman_encode(&huffman_tree, &result2, i);

        TEST_ASSERT_EQUAL(result.code, result2.code);
        TEST_ASSERT_EQUAL(result.length, result2.length);
    }
}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_hpack_huffman_encode_random);
    UNIT_TEST(test_hpack_huffman_encode_unique);
    UNIT_TEST(test_hpack_huffman_encode_twice);

    return UNITY_END();
}


