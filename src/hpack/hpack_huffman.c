//
// Created by Gabriel Norambuena on 18-07-19.
//
#include "hpack_huffman.h"

#ifdef INCLUDE_HUFFMAN_COMPRESSION
/*
 * This is a implementation of a compact huffman tree using the algorithm in
 * Compact Data Structures A Practical Approach(2016) By G. Navarro
 * L, F, C are precomputed
 * L_inverse is a reverse mapping of L for better performance while encoding
 * */
static const hpack_huffman_tree_t huffman_tree = {
        .L = {
                48, 49, 50, 97, 99, 101, 105, 111, 115, 116, 32, 37, 45, 46, 47, 51, 52, 53, 54, 55, 56, 57, 61, 65, 95,
                98, 100, 102, 103, 104, 108, 109, 110, 112, 114, 117, 58, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76,
                77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 89, 106, 107, 113, 118, 119, 120, 121, 122, 38, 42, 44, 59,
                88, 90, 33, 34, 40, 41, 63, 39, 43, 124, 35, 62, 0, 36, 64, 91, 93, 126, 94, 125, 60, 96, 123, 92, 195,
                208, 128, 130, 131, 162, 184, 194, 224, 226, 153, 161, 167, 172, 176, 177, 179, 209, 216, 217, 227, 229,
                230, 129, 132, 133, 134, 136, 146, 154, 156, 160, 163, 164, 169, 170, 173, 178, 181, 185, 186, 187, 189,
                190, 196, 198, 228, 232, 233, 1, 135, 137, 138, 139, 140, 141, 143, 147, 149, 150, 151, 152, 155, 157,
                158, 165, 166, 168, 174, 175, 180, 182, 183, 188, 191, 197, 231, 239, 9, 142, 144, 145, 148, 159, 171,
                206, 215, 225, 236, 237, 199, 207, 234, 235, 192, 193, 200, 201, 202, 205, 210, 213, 218, 219, 238, 240,
                242, 243, 255, 203, 204, 211, 212, 214, 221, 222, 223, 241, 244, 245, 246, 247, 248, 250, 251, 252, 253,
                254, 2, 3, 4, 5, 6, 7, 8, 11, 12, 14, 15, 16, 17, 18, 19, 20, 21, 23, 24, 25, 26, 27, 28, 29, 30, 31,
                127, 220, 249, 10, 13, 22
        },
        .L_inverse = {
                84, 145, 224, 225, 226, 227, 228, 229, 230, 174, 253, 231, 232, 254, 233, 234, 235, 236, 237, 238, 239,
                240, 255, 241, 242, 243, 244, 245, 246, 247, 248, 249, 10, 74, 75, 82, 85, 11, 68, 79, 76, 77, 69, 80,
                70, 12, 13, 14, 0, 1, 2, 15, 16, 17, 18, 19, 20, 21, 36, 71, 92, 22, 83, 78, 86, 23, 37, 38, 39, 40, 41,
                42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 72, 59, 73, 87, 95, 88, 90, 24, 93,
                3, 25, 4, 26, 5, 27, 28, 29, 6, 60, 61, 30, 31, 32, 7, 33, 62, 34, 8, 9, 35, 63, 64, 65, 66, 67, 94, 81,
                91, 89, 250, 98, 119, 99, 100, 120, 121, 122, 146, 123, 147, 148, 149, 150, 151, 175, 152, 176, 177,
                124, 153, 178, 154, 155, 156, 157, 106, 125, 158, 126, 159, 160, 179, 127, 107, 101, 128, 129, 161, 162,
                108, 163, 130, 131, 180, 109, 132, 164, 165, 110, 111, 133, 112, 166, 134, 167, 168, 102, 135, 136, 137,
                169, 138, 139, 170, 190, 191, 103, 96, 140, 171, 141, 186, 192, 193, 194, 205, 206, 195, 181, 187, 97,
                113, 196, 207, 208, 197, 209, 182, 114, 115, 198, 199, 251, 210, 211, 212, 104, 183, 105, 116, 142, 117,
                118, 172, 143, 144, 188, 189, 184, 185, 200, 173, 201, 213, 202, 203, 214, 215, 216, 217, 218, 252, 219,
                220, 221, 222, 223, 204
        },
        .F = {
                0, 0, 0, 0, 0, 0, 10, 36, 68, 74, 74, 79, 82, 84, 90, 92, 95, 95, 95, 95, 98, 106, 119, 145, 174, 186,
                190, 205, 224, 253, 253
        },
        .C = {
                0, 0, 0, 0, 0, 0, 20, 92, 248, 508, 1016, 2042, 4090, 8184, 16380, 32764, 65534, 131068, 262136, 524272,
                1048550, 2097116, 4194258, 8388568, 16777194, 33554412, 67108832, 134217694, 268435426, 536870910,
                1073741820
        }
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
int8_t hpack_huffman_encode(huffman_encoded_word_t *result, uint8_t sym) {
    if (huffman_tree.L_inverse[sym] >= huffman_tree.F[NUMBER_OF_CODE_LENGTHS - 1]) {
        result->code =
                huffman_tree.L_inverse[sym]
                - huffman_tree.F[NUMBER_OF_CODE_LENGTHS - 1]
                + huffman_tree.C[NUMBER_OF_CODE_LENGTHS - 1];
        result->length = NUMBER_OF_CODE_LENGTHS - 1;
        return 0;
    }

    for (int i = 1; i < NUMBER_OF_CODE_LENGTHS; i++) {
        if (huffman_tree.L_inverse[sym] < huffman_tree.F[i]) {
            result->code = huffman_tree.L_inverse[sym] - huffman_tree.F[i - 1] + huffman_tree.C[i - 1];
            result->length = i - 1;
            return 0;
        }
    }
    return -1;
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
int8_t hpack_huffman_decode(huffman_encoded_word_t *encoded, uint8_t *sym) {
    uint8_t length = 30;
    uint8_t h = NUMBER_OF_CODE_LENGTHS - 1;
    uint32_t code = encoded->code;

    for (uint8_t i = 5; i < h; i++) {
        uint32_t offset = (1 << (h - i - 1));
        uint32_t upper_bound = ((uint32_t) huffman_tree.C[i + 1]) * offset;
        if (code < upper_bound) {
            length = i;
            code >>= (h - i);
            break;
        }
    }
    uint32_t pos = huffman_tree.F[length] + code - huffman_tree.C[length];
    if (pos < HUFFMAN_TABLE_SIZE) {
        *sym = huffman_tree.L[pos];
        //TODO: Change this to return length
        encoded->length = length;
        return 0;
    }
    return -1;
}

#endif
