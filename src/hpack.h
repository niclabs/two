//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include <stdint.h>
#include "table.h"
#include "headers.h"
#include "hpack_encoder.h"
#include "hpack_decoder.h"
#include "hpack_tables.h"

/*
   int compress_huffman(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_static(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_dynamic(char* headers, int headers_size, uint8_t* compressed_headers);
 */

int decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int encode(hpack_dynamic_table_t *dynamic_table, char *name_string, char *value_string,  uint8_t *encoded_buffer);

int encode_dynamic_size_update(hpack_dynamic_table_t *dynamic_table, uint32_t max_size, uint8_t* encoded_buffer);

int hpack_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t* table);

uint32_t hpack_get_table_length(uint32_t dynamic_table_size);

#endif //TWO_HPACK_H
