#include "hpack_utils.h"
#include <stdint.h>
//
// Created by maite on 30-05-19.
//

/*
 * pass a byte b to string bits in str
 * "str" must be a char[9] for allocate string with 8chars + '/0'
 * Returns 0 if ok.
 */
int byte_to_8bits_string(uint8_t b, char *str)
{
    uint8_t aux = (uint8_t)128;

    for (uint8_t i = 0; i < 8; i++) {
        if (b & aux) {
            str[i] = '1';
        }
        else {
            str[i] = '0';
        }
        aux = aux >> 1;
    }
    str[8] = '\0';
    return 0;
}
/*
 * Function: hpack_utils_read_bits_from_bytes
 * Reads bits from a buffer of bytes (max number of bits it can read is 32).
 * Input:
 * -> current_bit_pointer: The bit from where to start reading (inclusive)
 * -> number_of_bits_to_read: The number of bits to read from the buffer
 * -> *buffer: The buffer containing the bits to read
 * -> buffer_size: Size of the buffer
 * output: returns the bits read from
 */
uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer, uint8_t buffer_size)
{
    uint32_t byte_offset = current_bit_pointer / 8;
    uint8_t bit_offset = current_bit_pointer - 8 * byte_offset;
    uint8_t num_bytes = ((number_of_bits_to_read + current_bit_pointer - 1) / 8) - (current_bit_pointer / 8) + 1;

    uint32_t mask = 1 << (8 * num_bytes - number_of_bits_to_read);
    mask -= 1;
    mask = ~mask;
    mask >>= bit_offset;
    mask &= ((1 << (8 * num_bytes - bit_offset)) - 1);
    uint32_t code = buffer[byte_offset];
    for (int i = 1; i < num_bytes; i++) {
        code <<= 8;
        code |= buffer[i + byte_offset];
    }
    code &= mask;
    code >>= (8 * num_bytes - number_of_bits_to_read - bit_offset);

    return code;
}

int8_t hpack_utils_check_if_can_read_buffer(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t buffer_size)
{
    uint32_t byte_offset = current_bit_pointer / 8;
    uint8_t bit_offset = current_bit_pointer - 8 * byte_offset;
    uint8_t num_bytes = ((number_of_bits_to_read + current_bit_pointer - 1) / 8) - (current_bit_pointer / 8) + 1;

    if (num_bytes + byte_offset > buffer_size) {
        return -1;
    }
    return 0;
}
