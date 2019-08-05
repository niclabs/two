//
// Created by maite on 30-05-19.
//

#ifndef TWO_HPACK_ENCODER_H
#define TWO_HPACK_ENCODER_H

#include "http_bridge.h"
#include <stdint.h>
#include "logging.h"
#include "headers.h"
#include "hpack_huffman.h"
#include "hpack_utils.h"
#include "hpack_tables.h"


int hpack_encoder_encode(hpack_preamble_t preamble, uint32_t max_size, char *name_string, char *value_string,  uint8_t *encoded_buffer);


#endif //TWO_HPACK_ENCODER_H
