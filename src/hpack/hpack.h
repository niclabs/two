//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_H
#define TWO_HPACK_H


#include <stdint.h>             /* for uint8_t, uint32_t    */

#include "header_list.h"        /* for header_list_t    */
#include "hpack/structs.h"      /* for hpack_states */



void hpack_init(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_table_size);

int hpack_decode(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, int header_block_size, header_list_t *headers);
int hpack_encode(hpack_dynamic_table_t *dynamic_table,
                 header_list_t *headers_out,
                 uint8_t *encoded_buffer,
                 uint32_t buffer_size);
void hpack_dynamic_change_max_size(hpack_dynamic_table_t *dynamic_table, uint32_t incoming_max_table_size);


#endif //TWO_HPACK_H
