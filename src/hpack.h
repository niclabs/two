//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include "http_bridge.h"
#include <stdint.h>
#include "logging.h"
#include "table.h"


typedef enum{
    INDEXED_HEADER_FIELD = (uint8_t)128,
    LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING = (uint8_t)64,
    LITERAL_HEADER_FIELD_WITHOUT_INDEXING = (uint8_t)0,
    LITERAL_HEADER_FIELD_NEVER_INDEXED = (uint8_t)16,
    DYNAMIC_TABLE_SIZE_UPDATE = (uint8_t)32
}hpack_preamble_t;



int compress_hauffman(char* headers, int headers_size, uint8_t* compressed_headers);
int compress_static(char* headers, int headers_size, uint8_t* compressed_headers);
int compress_dynamic(char* headers, int headers_size, uint8_t* compressed_headers);


int decode_header_block(uint8_t* header_block, uint8_t header_block_size, headers_lists_t* h_list);
int encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index,char* value_string, uint8_t value_huffman_bool, char* name_string, uint8_t name_huffman_bool, uint8_t* encoded_buffer);

#endif //TWO_HPACK_H
