//
// Created by gabriel on 18-07-19.
//

#ifndef HPACK_HUFFMAN_H
#define HPACK_HUFFMAN_H

#include <stdio.h>

#define HUFFMAN_TABLE_SIZE 257
#define NUMBER_OF_CODE_LENGTHS 21
#define INCLUDE_HUFFMAN_LENGTH_TABLE 1

typedef struct {
    uint32_t code;
    uint8_t length;
} huffman_encoded_word_t;

typedef struct {
    const uint32_t codes[HUFFMAN_TABLE_SIZE];
    const uint32_t symbols[HUFFMAN_TABLE_SIZE];
#ifdef INCLUDE_HUFFMAN_LENGTH_TABLE
    const uint8_t huffman_length[HUFFMAN_TABLE_SIZE];
#endif
    const uint8_t sR[NUMBER_OF_CODE_LENGTHS];
    const uint8_t code_length[NUMBER_OF_CODE_LENGTHS];
} huffman_tree_t;

/*void huffman_encode(const huffman_tree_t *huffman_tree, huffman_encoded_word_t *result, uint8_t sym)*/

/*uint8_t huffman_decode(const huffman_tree_t *huffman_tree, huffman_encoded_word_t *encoded)*/

#endif //HPACK_HUFFMAN_H
