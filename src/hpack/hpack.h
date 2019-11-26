//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include <stdint.h>             /* for uint8_t, uint32_t    */
#include "headers.h"            /* for header_list_t    */
#include "hpack/structs.h"      /* for hpack_states */



int decode_header_block(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, header_list_t *headers);

int encode(hpack_states_t *states, char *name_string, char *value_string,  uint8_t *encoded_buffer);

int encode_dynamic_size_update(hpack_states_t *states, uint32_t max_size, uint8_t *encoded_buffer);

void hpack_init_states(hpack_states_t *states, uint32_t settings_max_table_size);


#endif //TWO_HPACK_H
