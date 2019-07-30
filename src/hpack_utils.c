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
 * Before calling this function, the caller has to check if the number of bits to read from the buffer
 * don't exceed the size of the buffer, use hpack_utils_can_read_buffer to check this condition
 * Input:
 *      -> current_bit_pointer: The bit from where to start reading (inclusive)
 *      -> number_of_bits_to_read: The number of bits to read from the buffer
 *      -> *buffer: The buffer containing the bits to read
 *      -> buffer_size: Size of the buffer
 * output: returns the bits read from
 */
uint32_t hpack_utils_read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer)
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

/*
 * Function: hpack_utils_check_if_can_read_buffer
 * Checks if the number of bits to read from the buffer from the current_bit_pointer doesn't exceed the size of the buffer
 * This function is meant to be used to check if hpack_utils_read_bits_from_bytes can read succesfully from the buffer
 * Input:
 *      -> current_bit_pointer: Bit number from where to start reading in the buffer(this is not a byte pointer!)
 *      -> number_of_bits_to_read: Number of bits to read from the buffer starting from the current_bit_pointer (inclusive)
 *      -> buffer_size: Size of the buffer to read in bytes.
 * Output:
 *      Returns 0 if the required amount of bits can be read from the buffer, or -1 otherwise
 */
int8_t hpack_utils_check_can_read_buffer(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t buffer_size)
{
    if (current_bit_pointer + number_of_bits_to_read > 8 * buffer_size) {
        return -1;
    }
    return 0;
}

/*
 * Function: hpack_utils_log128
 * Compute the log128 of the input
 * Input:
 *      -> x: variable to apply log128
 * Output:
 *      returns log128(x)
 */
int hpack_utils_log128(uint32_t x)
{
    uint32_t n = 0;
    uint32_t m = 1;

    while (m < x) {
        m = 1 << (7 * (++n));
    }

    if (m == x) {
        return n;
    }
    return n - 1;
}