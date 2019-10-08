//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_ENCODER_H
#define TWO_HPACK_ENCODER_H

#include <stdint.h>             /* for uint8_t, uint32_t    */
#include "hpack/tables.h"       /* for hpack_find_index ...    */
#include "http_bridge.h"
#include "hpack/utils.h"        /* for hpack_utils_find_prefix_size, hpack_pr...*/


int hpack_encoder_encode(hpack_states_t *states, char *name_string, char *value_string,  uint8_t *encoded_buffer);
int hpack_encoder_encode_dynamic_size_update(hpack_states_t *states, uint32_t max_size, uint8_t *encoded_buffer);


#endif //TWO_HPACK_ENCODER_H
