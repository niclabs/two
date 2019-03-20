#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include "utils.h"

//TODO check big endian or little endian...

//number to uint8_t*
uint8_t* uint32toByteArray(uint32_t x){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*4);
    bytes[0] = (x >> 24) & 0xFF;
    bytes[1] = (x >> 16)  & 0xFF;
    bytes[2] = (x >> 8)  & 0xFF;
    bytes[3] = (x >> 0) & 0xFF;
    return bytes;
}

uint8_t* uint32_31toByteArray(uint32_t x){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*4);
    bytes[0] = (x >> 24) & 0xFF;
    uint8_t first_byte = (uint8_t)(((uint8_t)bytes[0])<<1)>>1; //remove first bit
    bytes[0] = first_byte;
    bytes[1] = (x >> 16)  & 0xFF;
    bytes[2] = (x >> 8)  & 0xFF;
    bytes[3] = (x >> 0) & 0xFF;
    return bytes;
}

uint8_t* uint32_24toByteArray(uint32_t x){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*3);
    bytes[0] = (x >> 16)  & 0xFF;
    bytes[1] = (x >> 8)  & 0xFF;
    bytes[2] = (x >> 0) & 0xFF;
    return bytes;
}


uint8_t* uint16toByteArray(uint16_t x){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*2);
    bytes[0] = (x >> 8)  & 0xFF;
    bytes[1] = (x >> 0)  & 0xFF;
    return bytes;
}

//byte to numbers
uint32_t bytesToUint32(uint8_t* bytes){
    uint32_t number = (bytes[0] << 24) + (bytes[1] << 16) + (bytes[2] << 8) + bytes[3];
    return number;
}

uint16_t bytesToUint16(uint8_t* bytes){
    uint16_t number = (bytes[0] << 8) + bytes[1];
    return number;
}

uint32_t bytesToUint32_24(uint8_t* bytes){
    uint32_t number =(bytes[0] << 16) + (bytes[1] << 8) + bytes[2];
    return number;
}

uint32_t bytesToUint32_31(uint8_t* bytes){
    uint8_t first_byte = (uint8_t)(bytes[0]<<1)>>1;//removes first bit
    uint32_t number = ((first_byte << 24) +(bytes[1] << 16) + (bytes[2] << 8) + bytes[3]);
    return number;
}

int appendByteArrays(uint8_t* dest, uint8_t* array1, uint8_t* array2, int size1, int size2){
    for(uint8_t i = 0; i < size1; i++){
        dest[i]=array1[i];
    }
    for(uint8_t i = 0; i < size2; i++){
        dest[size1+i]=array2[i];
    }
    //memcpy(total, array1, sizeof(array1));
    //memcpy(total+sizeof(array1), array2, sizeof(array2));
    return size1 + size2;
}