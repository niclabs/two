//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include <stdint.h>             /* for uint8_t, uint32_t    */
#include "headers.h"            /* for header_list_t    */
#include "hpack/structs.h"      /* for hpack_states */



void hpack_init_states(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_table_size);

int decode(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, int header_block_size, header_list_t *headers);
int encode(hpack_dynamic_table_t *dynamic_table, header_list_t *headers_out,  uint8_t *encoded_buffer);

int encode_dynamic_size_update(hpack_dynamic_table_t *dynamic_table, uint32_t max_size, uint8_t *encoded_buffer);


void hpack_dynamic_change_max_size(hpack_dynamic_table_t *dynamic_table, uint32_t incoming_max_table_size);


#endif //TWO_HPACK_H
