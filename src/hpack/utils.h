#ifndef TWO_HPACK_UTILS_H
#define TWO_HPACK_UTILS_H

#include "hpack/structs.h"


uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, const uint8_t *buffer);
uint32_t hpack_utils_log128(uint32_t x);
hpack_preamble_t hpack_utils_get_preamble(uint8_t preamble);
uint8_t hpack_utils_find_prefix_size(hpack_preamble_t octet);
uint32_t hpack_utils_encoded_integer_size(uint32_t num, uint8_t prefix);


#endif //TWO_HPACK_UTILS_H
