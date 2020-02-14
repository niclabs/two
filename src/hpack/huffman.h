//
// Created by gabriel on 18-07-19.
//

#ifndef HPACK_HUFFMAN_H
#define HPACK_HUFFMAN_H

#include <stdint.h>

#define HUFFMAN_TABLE_SIZE     (256)
#define NUMBER_OF_CODE_LENGTHS (31)

typedef struct
{
    const uint8_t L[HUFFMAN_TABLE_SIZE];
    const uint8_t L_inverse[HUFFMAN_TABLE_SIZE];
    const uint8_t F[NUMBER_OF_CODE_LENGTHS];
    const uint32_t C[NUMBER_OF_CODE_LENGTHS];
} hpack_huffman_tree_t;

typedef struct
{
    uint32_t code;
    uint8_t length;
} huffman_encoded_word_t;

/*
 * Function: hpack_huffman_encode
 * encodes the given symbol using the encoded representation stored in the
 * huffman_tree Input:
 *      -> *huffman_tree: Pointer to the huffman tree storing the encoded
 * symbols
 *      -> *result: Pointer to struct to save the result of the encoding
 *      -> sym: Symbol to encode
 * Output: 0 if it can encode the given symbol, -1 otherwise
 */
int8_t hpack_huffman_encode(huffman_encoded_word_t *result, uint8_t sym);

/*
 * Function: hpack_huffman_decode
 * Decodes the given encoded word and stores the result in sym
 * Input:
 *      -> *encoded: Struct containing encoded word
 *      -> *sym: Byte to store result
 * Output:
 *      Returns the number of bits read if successful, if it cannot find the
 * symbol it returns -1
 */
int8_t hpack_huffman_decode(huffman_encoded_word_t *encoded, uint8_t *sym);

#endif // HPACK_HUFFMAN_H
