#include <stdint.h>

#include "buffer.h"

uint32_t buffer_get_u32(uint8_t *bytes)
{
    return (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
}

uint32_t buffer_get_u31(uint8_t *bytes)
{
    return ((bytes[0] & 0x7F) << 24) + (bytes[1] << 16) + (bytes[2] << 8) +
           bytes[3];
}

uint32_t buffer_get_u24(uint8_t *bytes)
{
    return (bytes[0] << 16) + (bytes[1] << 8) + bytes[2];
}

uint16_t buffer_get_u16(uint8_t *bytes)
{
    return (bytes[0] << 8) + bytes[1];
}

uint8_t buffer_get_u8(uint8_t *bytes)
{
    return bytes[0];
}

void buffer_put_u32(uint8_t *bytes, uint32_t x)
{
    bytes[0] = (x >> 24) & 0xFF;
    bytes[1] = (x >> 16) & 0xFF;
    bytes[2] = (x >> 8) & 0xFF;
    bytes[3] = (x >> 0) & 0xFF;
}

void buffer_put_u31(uint8_t *bytes, uint32_t x)
{

    bytes[0] = (x >> 24) & 0x7F;
    bytes[1] = (x >> 16) & 0xFF;
    bytes[2] = (x >> 8) & 0xFF;
    bytes[3] = (x >> 0) & 0xFF;
}

void buffer_put_u24(uint8_t *bytes, uint32_t x)
{
    bytes[0] = (x >> 16) & 0xFF;
    bytes[1] = (x >> 8) & 0xFF;
    bytes[2] = (x >> 0) & 0xFF;
}

void buffer_put_u16(uint8_t *bytes, uint16_t x)
{
    bytes[0] = (x >> 8) & 0xFF;
    bytes[1] = (x >> 0) & 0xFF;
}

void buffer_put_u8(uint8_t *bytes, uint8_t x)
{
    bytes[0] = x;
}
