//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H

#include "http_bridge.h"
#include <stdint.h>
#include "logging.h"
#include "table.h"
#include "headers.h"
#include "hpack_huffman.h"
#include "hpack_utils.h"
#include "hpack_encoder.h"
#include "hpack_tables.h"


/*
   int compress_huffman(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_static(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_dynamic(char* headers, int headers_size, uint8_t* compressed_headers);
 */

int decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index, char *name_string, uint8_t name_huffman_bool, char *value_string, uint8_t value_huffman_bool,  uint8_t *encoded_buffer);

int hpack_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t* table);

uint32_t hpack_get_table_length(uint32_t dynamic_table_size);

#endif //TWO_HPACK_H
