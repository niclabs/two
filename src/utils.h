#ifndef UTILS_H
#define UTILS_H

#include <stdint.h>

int uint32_to_byte_array(uint32_t x, uint8_t* bytes);

int uint32_31_to_byte_array(uint32_t x, uint8_t* bytes);

int uint32_24_to_byte_array(uint32_t x, uint8_t* bytes);


int uint16_to_byte_array(uint16_t x, uint8_t* bytes);

//byte to numbers
uint32_t bytes_to_uint32(uint8_t* bytes);

uint16_t bytes_to_uint16(uint8_t* bytes);

uint32_t bytes_to_uint32_24(uint8_t* bytes);

uint32_t bytes_to_uint32_31(uint8_t* bytes);


int append_byte_arrays(uint8_t* dest, uint8_t* array1, uint8_t* array2, int size1, int size2);

//int buffer_copy(uint8_t* dest, uint8_t* orig, int size);

#endif
