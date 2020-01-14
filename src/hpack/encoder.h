//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_ENCODER_H
#define TWO_HPACK_ENCODER_H

#include <stdint.h>         /* for uint8_t, uint32_t    */
#include "hpack.h"
#include "tables.h"         /* for hpack_dynamic_table_t    */
#include "config.h"


int hpack_encoder_encode(hpack_dynamic_table_t *dynamic_table,
                         header_list_t *headers_out,
                         uint8_t *encoded_buffer,
                         uint32_t buffer_size);
int hpack_encoder_encode_dynamic_size_update(hpack_dynamic_table_t *dynamic_table, uint32_t max_size, uint8_t *encoded_buffer);


#endif //TWO_HPACK_ENCODER_H
