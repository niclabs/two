#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

/**
 * Get first 4 bytes of the byte buffer as uint32
 *
 * @param bytes byte buffer
 * @return unsigned int from the beginning of the buffer
 */
uint32_t buffer_get_u32(uint8_t *bytes);

/**
 * Get first 4 bytes of the byte buffer ignoring reserved first bit as uint32
 *
 * @param bytes byte buffer
 * @return uint32 from the beginning of the buffer
 */
uint32_t buffer_get_u31(uint8_t *bytes);

/**
 * Get first 3 bytes of the byte buffer as a uint32
 *
 * @param bytes byte buffer
 * @return uint32 representation of the first 3 bytes
 */
uint32_t buffer_get_u24(uint8_t *bytes);

/**
 * Get first 2 bytes of the buffer as a uint16
 *
 * @param bytes byte buffer
 * @return uint16 representation of the first 2 bytes
 */
uint16_t buffer_get_u16(uint8_t *bytes);

/**
 * Get first byte of the buffer
 *
 * @param bytes byte buffer
 * @return first byte
 */
uint8_t buffer_get_u8(uint8_t *bytes);

/**
 * Set the first 4 bytes of the buffer with the value of the given uint32 param
 *
 * @param bytes byte buffer
 * @param x uint32 to set
 */
void buffer_put_u32(uint8_t *bytes, uint32_t x);

/**
 * Set the first 4 bytes of the buffer using the least significant 31-bits
 * of the given uint32 param
 *
 * @param bytes byte buffer
 * @param x uint32 to set, only the least significant 31-bits will be used
 */
void buffer_put_u31(uint8_t *bytes, uint32_t x);

/**
 * Set the first 3 bytes of the buffer using the least significant 24-bits
 * of the given uint32 param
 *
 * @param bytes byte buffer
 * @param x uint32 to set, only the least significant 24 bits will be used
 */
void buffer_put_u24(uint8_t *bytes, uint32_t x);

/**
 * Set the first 2 bytes of the buffer with the value of the given uint16 param
 *
 * @param bytes byte buffer
 * @param x uint16 to set
 */
void buffer_put_u16(uint8_t *bytes, uint16_t x);

/**
 * Set the first byte of the buffer to the given value
 *
 * @param bytes byte buffer
 * @param x byte
 */
void buffer_put_u8(uint8_t *bytes, uint8_t x);

#endif
