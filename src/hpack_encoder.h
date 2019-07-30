//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_ENCODER_H
#define TWO_HPACK_ENCODER_H

#include "http_bridge.h"
#include <stdint.h>
#include "logging.h"
#include "table.h"
#include "headers.h"
#include "hpack_huffman.h"
#include "hpack_utils.h"


int hpack_encoder_encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index, char *name_string, uint8_t name_huffman_bool, char *value_string, uint8_t value_huffman_bool,  uint8_t *encoded_buffer);


#endif //TWO_HPACK_ENCODER_H
