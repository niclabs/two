
#ifndef TWO_HPACK_STRUCTS
#define TWO_HPACK_STRUCTS

#include <stdint.h>             /* for int8_t, uint8_t, uint32_t, uint16_t*/

// library configuration
#include "two-conf.h"

#ifndef HPACK_MAXIMUM_INTEGER
#define HPACK_MAXIMUM_INTEGER (4096)
#endif

#ifndef HTTP2_HEADER_TABLE_SIZE
#define HPACK_INCLUDE_DYNAMIC_TABLE (1)
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (4096)
#elif (HTTP2_HEADER_TABLE_SIZE) == 0
#define HPACK_INCLUDE_DYNAMIC_TABLE (0)
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (0)
#else
#define HPACK_INCLUDE_DYNAMIC_TABLE (1)
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (HTTP2_HEADER_TABLE_SIZE)
#endif

#ifndef HPACK_HEADER_NAME_LEN
#define HPACK_HEADER_NAME_LEN (32)
#endif

#ifndef HPACK_HEADER_VALUE_LEN
#define HPACK_HEADER_VALUE_LEN (64)
#endif

typedef enum {
    INDEXED_HEADER_FIELD                            = (uint8_t) 128,
    LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING  = (uint8_t) 64,
    LITERAL_HEADER_FIELD_WITHOUT_INDEXING           = (uint8_t) 0,
    LITERAL_HEADER_FIELD_NEVER_INDEXED              = (uint8_t) 16,
    DYNAMIC_TABLE_SIZE_UPDATE                       = (uint8_t) 32
} hpack_preamble_t;

typedef struct {
    uint32_t index;
    uint32_t name_length;
    uint32_t value_length;
    uint8_t *name_string;
    uint8_t *value_string;
    uint16_t dynamic_table_size;
    uint8_t huffman_bit_of_name;
    uint8_t huffman_bit_of_value;
    hpack_preamble_t preamble;
} hpack_encoded_header_t;

//typedefs for dinamic
//size: 10 bytes in params + size of buffer
typedef
#if HPACK_INCLUDE_DYNAMIC_TABLE
    struct {
    uint16_t max_size;
    uint16_t first;
    uint16_t next;
    uint16_t actual_size;
    uint16_t n_entries;
    uint32_t settings_max_table_size;
    char buffer[HPACK_MAX_DYNAMIC_TABLE_SIZE];
}
#else
    char //case No dynamic table mode, this conversion to char makes the code cleaner
#endif
hpack_dynamic_table_t;

typedef enum {
    HPACK_NO_ERROR          = (int8_t)  0,
    HPACK_COMPRESSION_ERROR = (int8_t) -1,
    HPACK_INTERNAL_ERROR    = (int8_t) -2,
    HPACK_MEMORY_ERROR      = (int8_t) -3
} hpack_error_t;

#endif
