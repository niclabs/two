//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_ENCODER_H
#define TWO_HPACK_ENCODER_H

#include <stdint.h>             /* for uint8_t, uint32_t    */
#include "hpack.h"
#include "hpack_tables.h"       /* for hpack_dynamic_table_t    */
#include "config.h"


int hpack_encoder_encode(hpack_states_t *states, char *name_string, char *value_string,  uint8_t *encoded_buffer);
int hpack_encoder_encode_dynamic_size_update(hpack_states_t *states, uint32_t max_size, uint8_t *encoded_buffer);


#endif //TWO_HPACK_ENCODER_H
