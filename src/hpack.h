//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H



#include <stdint.h>



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



#endif //TWO_HPACK_H
