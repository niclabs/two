#ifndef TWO_HPACK_DECODER_H
#define TWO_HPACK_DECODER_H

#include "http_bridge.h"
#include <stdint.h>
#include "logging.h"
#include "headers.h"
#include "hpack_huffman.h"
#include "hpack_utils.h"
#include "hpack_tables.h"


int hpack_decoder_decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

int hpack_decoder_decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

#endif //TWO_HPACK_DECODER_H