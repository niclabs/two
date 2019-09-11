//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include <stdint.h>             /* for uint8_t, uint32_t    */
#include "headers.h"            /* for headers_t    */
#include "hpack_utils.h"        /* for hpack_states */


/*
   int compress_huffman(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_static(char* headers, int headers_size, uint8_t* compressed_headers);
   int compress_dynamic(char* headers, int headers_size, uint8_t* compressed_headers);
 */

int decode_header_block(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

//int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int encode(hpack_states_t *states, char *name_string, char *value_string,  uint8_t *encoded_buffer);

int encode_dynamic_size_update(hpack_states_t *states, uint32_t max_size, uint8_t *encoded_buffer);

int8_t hpack_init_states(hpack_states_t *states, uint32_t settings_max_table_size);


#endif //TWO_HPACK_H
