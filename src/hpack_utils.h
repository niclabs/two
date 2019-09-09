#ifndef TWO_HPACK_UTILS_H
#define TWO_HPACK_UTILS_H

#include <stdint.h>             /* for int8_t, uint8_t, uint32_t, uint16_t*/
//TODO move macro to conf file
#define HPACK_CONF_MAXIMUM_INTEGER_SIZE (4096) //DEFAULT

#ifdef HPACK_CONF_MAXIMUM_INTEGER_SIZE
#define HPACK_MAXIMUM_INTEGER_SIZE (HPACK_CONF_MAXIMUM_INTEGER_SIZE)
#else
#define HPACK_MAXIMUM_INTEGER_SIZE (4096)
#endif
typedef enum {
    INDEXED_HEADER_FIELD                            = (uint8_t) 128,
    LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING  = (uint8_t) 64,
    LITERAL_HEADER_FIELD_WITHOUT_INDEXING           = (uint8_t) 0,
    LITERAL_HEADER_FIELD_NEVER_INDEXED              = (uint8_t) 16,
    DYNAMIC_TABLE_SIZE_UPDATE                       = (uint8_t) 32
} hpack_preamble_t;


uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer);
int hpack_utils_log128(uint32_t x);
hpack_preamble_t hpack_utils_get_preamble(uint8_t preamble);
uint8_t hpack_utils_find_prefix_size(hpack_preamble_t octet);
uint32_t hpack_utils_encoded_integer_size(uint32_t num, uint8_t prefix);


#endif //TWO_HPACK_UTILS_H
