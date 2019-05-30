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
int byte_to_8bits_string(uint8_t b, char* str){
    uint8_t aux = (uint8_t)128;
    for(uint8_t i = 0; i < 8; i++){
        if(b & aux){
            str[i] = '1';
        }else{
            str[i] = '0';
        }
        aux = aux >> 1;
    }
    str[8] = '\0';
    return 0;
};