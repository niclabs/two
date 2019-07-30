//
// Created by maite on 30-05-19.
//
#include <stdint.h>
#ifndef TWO_HPACK_UTILS_H
#define TWO_HPACK_UTILS_H

int byte_to_8bits_string(uint8_t b, char* str);
uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer);
int8_t hpack_utils_check_can_read_buffer(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t buffer_size);
int hpack_utils_log128(uint32_t x);
#endif //TWO_HPACK_UTILS_H
