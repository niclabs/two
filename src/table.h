#include "http_bridge.h"

#ifndef TABLE_H
#define TABLE_H


/*--------------------HTTP2 structures and static values--------------------*/

#define HTTP2_MAX_HEADER_COUNT 32
#define HTTP2_MAX_HBF_BUFFER 128

// Key value pair
typedef struct TABLE_ENTRY {
    char name [32];
    char value [128];
} table_pair_t;


// Key value pair
typedef struct KEY_POINTER_MAP {
    char name [64];
    void * ptr;
} key_pointer_map_t;

typedef struct HEADERS_LISTS_S {
    uint8_t header_list_count_in;
    table_pair_t header_list_in[HTTP2_MAX_HEADER_COUNT];
    uint8_t header_list_count_out;
    table_pair_t header_list_out[HTTP2_MAX_HEADER_COUNT];
} headers_lists_t;


#endif /* TABLE_H */
