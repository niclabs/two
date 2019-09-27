#ifndef TWO_HPACK_UTILS_H
#define TWO_HPACK_UTILS_H

#include <stdint.h>             /* for int8_t, uint8_t, uint32_t, uint16_t*/
#include "headers.h"            /* for MAX_HEADER_SETTINGS */

//TODO move macro to conf file
#define HPACK_CONF_MAXIMUM_INTEGER_SIZE (4096) //DEFAULT

#ifdef HPACK_CONF_MAXIMUM_INTEGER_SIZE
#define HPACK_MAXIMUM_INTEGER_SIZE (HPACK_CONF_MAXIMUM_INTEGER_SIZE)
#else
#define HPACK_MAXIMUM_INTEGER_SIZE (4096)
#endif
#ifdef HPACK_CONF_INCLUDE_DYNAMIC_TABLE
#define HPACK_INCLUDE_DYNAMIC_TABLE (HPACK_CONF_INCLUDE_DYNAMIC_TABLE)
#endif

#ifdef HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE)
#else
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (4096)
#endif

#define HPACK_INCLUDE_DYNAMIC_TABLE (1)

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
    uint8_t huffman_bit_of_name;
    uint8_t huffman_bit_of_value;
    uint16_t dynamic_table_size;
    hpack_preamble_t preamble;
} hpack_encoded_header_t;

//typedefs for dinamic
//size: 10 bytes in params + size of buffer
typedef
    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    struct hpack_dynamic_table {
    uint16_t max_size;
    uint16_t first;
    uint16_t next;
    uint16_t actual_size;
    uint16_t n_entries;
    char buffer[HPACK_MAX_DYNAMIC_TABLE_SIZE];
}
    #else
    char //case No dynamic table mode, this conversion to char makes the code cleaner
    #endif
hpack_dynamic_table_t;

/* Hpack Struct: Hpack states
    It contains all the required structs or fields for hpack functionality
 */
typedef struct {
    hpack_dynamic_table_t dynamic_table;
    hpack_encoded_header_t encoded_header;
    char tmp_name[MAX_HEADER_NAME_LEN];
    char tmp_value[MAX_HEADER_VALUE_LEN];
    uint32_t settings_max_table_size;
} hpack_states_t;

typedef enum {
    PROTOCOL_ERROR  = (int8_t) -1,
    INTERNAL_ERROR  = (int8_t) -2
} hpack_error_t;


uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer);
int hpack_utils_log128(uint32_t x);
hpack_preamble_t hpack_utils_get_preamble(uint8_t preamble);
uint8_t hpack_utils_find_prefix_size(hpack_preamble_t octet);
uint32_t hpack_utils_encoded_integer_size(uint32_t num, uint8_t prefix);


#endif //TWO_HPACK_UTILS_H
