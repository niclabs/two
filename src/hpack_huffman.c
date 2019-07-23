//
// Created by gabriel on 18-07-19.
//
#include "hpack_huffman.h"

static const huffman_tree_t huffman_tree = {
    .codes = {
        0x1ff8u,
        0x7fffd8u,
        0xfffffe2u,
        0xfffffe3u,
        0xfffffe4u,
        0xfffffe5u,
        0xfffffe6u,
        0xfffffe7u,
        0xfffffe8u,
        0xffffeau,
        0x3ffffffcu,
        0xfffffe9u,
        0xfffffeau,
        0x3ffffffdu,
        0xfffffebu,
        0xfffffecu,
        0xfffffedu,
        0xfffffeeu,
        0xfffffefu,
        0xffffff0u,
        0xffffff1u,
        0xffffff2u,
        0x3ffffffeu,
        0xffffff3u,
        0xffffff4u,
        0xffffff5u,
        0xffffff6u,
        0xffffff7u,
        0xffffff8u,
        0xffffff9u,
        0xffffffau,
        0xffffffbu,
        0x14u,
        0x3f8u,
        0x3f9u,
        0xffau,
        0x1ff9u,
        0x15u,
        0xf8u,
        0x7fau,
        0x3fau,
        0x3fbu,
        0xf9u,
        0x7fbu,
        0xfau,
        0x16u,
        0x17u,
        0x18u,
        0x0u,
        0x1u,
        0x2u,
        0x19u,
        0x1au,
        0x1bu,
        0x1cu,
        0x1du,
        0x1eu,
        0x1fu,
        0x5cu,
        0xfbu,
        0x7ffcu,
        0x20u,
        0xffbu,
        0x3fcu,
        0x1ffau,
        0x21u,
        0x5du,
        0x5eu,
        0x5fu,
        0x60u,
        0x61u,
        0x62u,
        0x63u,
        0x64u,
        0x65u,
        0x66u,
        0x67u,
        0x68u,
        0x69u,
        0x6au,
        0x6bu,
        0x6cu,
        0x6du,
        0x6eu,
        0x6fu,
        0x70u,
        0x71u,
        0x72u,
        0xfcu,
        0x73u,
        0xfdu,
        0x1ffbu,
        0x7fff0u,
        0x1ffcu,
        0x3ffcu,
        0x22u,
        0x7ffdu,
        0x3u,
        0x23u,
        0x4u,
        0x24u,
        0x5u,
        0x25u,
        0x26u,
        0x27u,
        0x6u,
        0x74u,
        0x75u,
        0x28u,
        0x29u,
        0x2au,
        0x7u,
        0x2bu,
        0x76u,
        0x2cu,
        0x8u,
        0x9u,
        0x2du,
        0x77u,
        0x78u,
        0x79u,
        0x7au,
        0x7bu,
        0x7ffeu,
        0x7fcu,
        0x3ffdu,
        0x1ffdu,
        0xffffffcu,
        0xfffe6u,
        0x3fffd2u,
        0xfffe7u,
        0xfffe8u,
        0x3fffd3u,
        0x3fffd4u,
        0x3fffd5u,
        0x7fffd9u,
        0x3fffd6u,
        0x7fffdau,
        0x7fffdbu,
        0x7fffdcu,
        0x7fffddu,
        0x7fffdeu,
        0xffffebu,
        0x7fffdfu,
        0xffffecu,
        0xffffedu,
        0x3fffd7u,
        0x7fffe0u,
        0xffffeeu,
        0x7fffe1u,
        0x7fffe2u,
        0x7fffe3u,
        0x7fffe4u,
        0x1fffdcu,
        0x3fffd8u,
        0x7fffe5u,
        0x3fffd9u,
        0x7fffe6u,
        0x7fffe7u,
        0xffffefu,
        0x3fffdau,
        0x1fffddu,
        0xfffe9u,
        0x3fffdbu,
        0x3fffdcu,
        0x7fffe8u,
        0x7fffe9u,
        0x1fffdeu,
        0x7fffeau,
        0x3fffddu,
        0x3fffdeu,
        0xfffff0u,
        0x1fffdfu,
        0x3fffdfu,
        0x7fffebu,
        0x7fffecu,
        0x1fffe0u,
        0x1fffe1u,
        0x3fffe0u,
        0x1fffe2u,
        0x7fffedu,
        0x3fffe1u,
        0x7fffeeu,
        0x7fffefu,
        0xfffeau,
        0x3fffe2u,
        0x3fffe3u,
        0x3fffe4u,
        0x7ffff0u,
        0x3fffe5u,
        0x3fffe6u,
        0x7ffff1u,
        0x3ffffe0u,
        0x3ffffe1u,
        0xfffebu,
        0x7fff1u,
        0x3fffe7u,
        0x7ffff2u,
        0x3fffe8u,
        0x1ffffecu,
        0x3ffffe2u,
        0x3ffffe3u,
        0x3ffffe4u,
        0x7ffffdeu,
        0x7ffffdfu,
        0x3ffffe5u,
        0xfffff1u,
        0x1ffffedu,
        0x7fff2u,
        0x1fffe3u,
        0x3ffffe6u,
        0x7ffffe0u,
        0x7ffffe1u,
        0x3ffffe7u,
        0x7ffffe2u,
        0xfffff2u,
        0x1fffe4u,
        0x1fffe5u,
        0x3ffffe8u,
        0x3ffffe9u,
        0xffffffdu,
        0x7ffffe3u,
        0x7ffffe4u,
        0x7ffffe5u,
        0xfffecu,
        0xfffff3u,
        0xfffedu,
        0x1fffe6u,
        0x3fffe9u,
        0x1fffe7u,
        0x1fffe8u,
        0x7ffff3u,
        0x3fffeau,
        0x3fffebu,
        0x1ffffeeu,
        0x1ffffefu,
        0xfffff4u,
        0xfffff5u,
        0x3ffffeau,
        0x7ffff4u,
        0x3ffffebu,
        0x7ffffe6u,
        0x3ffffecu,
        0x3ffffedu,
        0x7ffffe7u,
        0x7ffffe8u,
        0x7ffffe9u,
        0x7ffffeau,
        0x7ffffebu,
        0xffffffeu,
        0x7ffffecu,
        0x7ffffedu,
        0x7ffffeeu,
        0x7ffffefu,
        0x7fffff0u,
        0x3ffffeeu
    },
    .symbols = {
        /*Codes of length 5*/
        48,
        49,
        50,
        97,
        99,
        101,
        105,
        111,
        115,
        116,
        /*Codes of length 6*/
        32,
        37,
        45,
        46,
        47,
        51,
        52,
        53,
        54,
        55,
        56,
        57,
        61,
        65,
        95,
        98,
        100,
        102,
        103,
        104,
        108,
        109,
        110,
        112,
        114,
        117,
        /*Codes of length 7*/
        58,
        66,
        67,
        68,
        69,
        70,
        71,
        72,
        73,
        74,
        75,
        76,
        77,
        78,
        79,
        80,
        81,
        82,
        83,
        84,
        85,
        86,
        87,
        89,
        106,
        107,
        113,
        118,
        119,
        120,
        121,
        122,
        /*Codes of length 8*/
        38,
        42,
        44,
        59,
        88,
        90,
        /*Codes of length 10*/
        33,
        34,
        40,
        41,
        63,
        /*Codes of length 11*/
        39,
        43,
        124,
        /*Codes of length 12*/
        35,
        62,
        /*Codes of length 13*/
        0,
        36,
        64,
        91,
        93,
        126,
        /*Codes of length 14*/
        94,
        125,
        /*Codes of length 15*/
        60,
        96,
        123,
        /*Codes of length 19*/
        92,
        195,
        208,
        /*Codes of length 20*/
        128,
        130,
        131,
        162,
        184,
        194,
        224,
        226,
        /*Codes of length 21*/
        153,
        161,
        167,
        172,
        176,
        177,
        179,
        209,
        216,
        217,
        227,
        229,
        230,
        /*Codes of length 22*/
        129,
        132,
        133,
        134,
        136,
        146,
        154,
        156,
        160,
        163,
        164,
        169,
        170,
        173,
        178,
        181,
        185,
        186,
        187,
        189,
        190,
        196,
        198,
        228,
        232,
        233,
        /*Codes of length 23*/
        1,
        135,
        137,
        138,
        139,
        140,
        141,
        143,
        147,
        149,
        150,
        151,
        152,
        155,
        157,
        158,
        165,
        166,
        168,
        174,
        175,
        180,
        182,
        183,
        188,
        191,
        197,
        231,
        239,
        /*Codes of length 24*/
        9,
        142,
        144,
        145,
        148,
        159,
        171,
        206,
        215,
        225,
        236,
        237,
        /*Codes of length 25*/
        199,
        207,
        234,
        235,
        /*Codes of length 26*/
        192,
        193,
        200,
        201,
        202,
        205,
        210,
        213,
        218,
        219,
        238,
        240,
        242,
        243,
        255,
        /*Codes of length 27*/
        203,
        204,
        211,
        212,
        214,
        221,
        222,
        223,
        241,
        244,
        245,
        246,
        247,
        248,
        250,
        251,
        252,
        253,
        254,
        /*Codes of length 28*/
        2,
        3,
        4,
        5,
        6,
        7,
        8,
        11,
        12,
        14,
        15,
        16,
        17,
        18,
        19,
        20,
        21,
        23,
        24,
        25,
        26,
        27,
        28,
        29,
        30,
        31,
        127,
        220,
        249,
        /*Codes of length 30*/
        10,
        13,
        22,
    },
#ifdef INCLUDE_HUFFMAN_LENGTH_TABLE
    .huffman_length = {
        13,
        23,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        24,
        30,
        28,
        28,
        30,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        30,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        28,
        6,
        10,
        10,
        12,
        13,
        6,
        8,
        11,
        10,
        10,
        8,
        11,
        8,
        6,
        6,
        6,
        5,
        5,
        5,
        6,
        6,
        6,
        6,
        6,
        6,
        6,
        7,
        8,
        15,
        6,
        12,
        10,
        13,
        6,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        7,
        8,
        7,
        8,
        13,
        19,
        13,
        14,
        6,
        15,
        5,
        6,
        5,
        6,
        5,
        6,
        6,
        6,
        5,
        7,
        7,
        6,
        6,
        6,
        5,
        6,
        7,
        6,
        5,
        5,
        6,
        7,
        7,
        7,
        7,
        7,
        15,
        11,
        14,
        13,
        28,
        20,
        22,
        20,
        20,
        22,
        22,
        22,
        23,
        22,
        23,
        23,
        23,
        23,
        23,
        24,
        23,
        24,
        24,
        22,
        23,
        24,
        23,
        23,
        23,
        23,
        21,
        22,
        23,
        22,
        23,
        23,
        24,
        22,
        21,
        20,
        22,
        22,
        23,
        23,
        21,
        23,
        22,
        22,
        24,
        21,
        22,
        23,
        23,
        21,
        21,
        22,
        21,
        23,
        22,
        23,
        23,
        20,
        22,
        22,
        22,
        23,
        22,
        22,
        23,
        26,
        26,
        20,
        19,
        22,
        23,
        22,
        25,
        26,
        26,
        26,
        27,
        27,
        26,
        24,
        25,
        19,
        21,
        26,
        27,
        27,
        26,
        27,
        24,
        21,
        21,
        26,
        26,
        28,
        27,
        27,
        27,
        20,
        24,
        20,
        21,
        22,
        21,
        21,
        23,
        22,
        22,
        25,
        25,
        24,
        24,
        26,
        23,
        26,
        27,
        26,
        26,
        27,
        27,
        27,
        27,
        27,
        28,
        27,
        27,
        27,
        27,
        27,
        26
    },
#endif
    .sR = {
        0,                      /*Codes of length 5*/
        10,                     /*Codes of length 6*/
        36,                     /*Codes of length 7*/
        68,                     /*Codes of length 8*/
        74,                     /*Codes of length 10*/
        79,                     /*Codes of length 11*/
        82,                     /*Codes of length 12*/
        84,                     /*Codes of length 13*/
        90,                     /*Codes of length 14*/
        92,                     /*Codes of length 15*/
        95,                     /*Codes of length 19*/
        98,                     /*Codes of length 20*/
        106,                    /*Codes of length 21*/
        119,                    /*Codes of length 22*/
        145,                    /*Codes of length 23*/
        174,                    /*Codes of length 24*/
        186,                    /*Codes of length 25*/
        190,                    /*Codes of length 26*/
        205,                    /*Codes of length 27*/
        224,                    /*Codes of length 28*/
        254,                    /*Codes of length 30*/
    },
    .code_length = {
        5,
        6,
        7,
        8,
        10,
        11,
        12,
        13,
        14,
        15,
        19,
        20,
        21,
        22,
        23,
        24,
        25,
        26,
        27,
        28,
        30,
    },
};

/*
 * Function: hpack_huffman_encode
 * encodes the given symbol using the encoded representation stored in the huffman_tree
 * Input:
 *      -> *huffman_tree: Pointer to the huffman tree storing the encoded symbols
 *      -> *result: Pointer to struct to save the result of the encoding
 *      -> sym: Symbol to encode
 * Output: 0 if it can encode the given symbol, -1 otherwise
 */
int8_t hpack_huffman_encode(huffman_encoded_word_t *result, uint8_t sym)
{
    result->code = huffman_tree.codes[sym];
#ifdef INCLUDE_HUFFMAN_LENGTH_TABLE
    result->length = huffman_tree.huffman_length[sym];
    return 0;
#else
    for (uint8_t i = 0; i < NUMBER_OF_CODE_LENGTHS; i++) {
        uint8_t symbol_index = huffman_tree.sR[i];
        uint16_t top = i + 1 < NUMBER_OF_CODE_LENGTHS ? (huffman_tree.sR[i + 1]) : HUFFMAN_TABLE_SIZE;

        for (uint8_t j = symbol_index; j < top; j++) {
            if (huffman_tree.symbols[j] == sym) {
                result->length = huffman_tree.code_length[i];
                return 0;
            }
        }
    }
    return -1;
#endif
}

/*
 * Function: hpack_huffman_decode
 * Decodes the given encoded word and stores the result in sym
 * Input:
 *      -> *encoded: Struct containing encoded word
 *      -> *sym: Byte to store result
 * Output:
 *      Returns 0 if successful, if it cannot find the symbol it returns -1
 */
int8_t hpack_huffman_decode(huffman_encoded_word_t *encoded, uint8_t* sym){
    uint8_t index = 0;
    for (uint8_t i = 0; i < NUMBER_OF_CODE_LENGTHS; i++) {
        if (huffman_tree.code_length[i] == encoded->length) {
            index = i;
            break;
        }
    }

    uint8_t symbol_index = huffman_tree.sR[index];
    uint16_t top = index + 1 < NUMBER_OF_CODE_LENGTHS ? (huffman_tree.sR[index + 1]) : HUFFMAN_TABLE_SIZE;

    for (uint8_t i = symbol_index; i < top; i++) {
        uint8_t code_index = huffman_tree.symbols[i];
        if (huffman_tree.codes[code_index] == encoded->code) {
            *sym = code_index;
            return 0;
        }
    }
    return -1;
}

