#ifndef TWO_HPACK_DECODER_H
#define TWO_HPACK_DECODER_H

#include <stdint.h>             /* for uint8_t */
#include "headers.h"            /* for headers_t */
#include "hpack_tables.h"       /* for hpack_dynamic_table_t */
#include "hpack_utils.h"        /* for hpack_preamble_t*/

typedef struct{
    uint32_t index;
    uint32_t name_length;
    uint32_t value_length;
    uint8_t* name_string;
    uint8_t* value_string;
    uint8_t huffman_bit_of_name;
    uint8_t huffman_bit_of_value;
    uint16_t dynamic_table_size;
    hpack_preamble_t preamble;
} hpack_encoded_header_t;

int hpack_decoder_decode_header_block(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int hpack_decoder_decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

#endif //TWO_HPACK_DECODER_H