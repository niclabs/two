#ifndef TWO_HPACK_DECODER_H
#define TWO_HPACK_DECODER_H

#include <stdint.h>             /* for uint8_t */
#include "headers.h"            /* for headers_t */
#include "hpack_tables.h"       /* for hpack_dynamic_table_t */
#include "hpack_utils.h"        /* for hpack_states*/



int hpack_decoder_decode_header_block(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

//int hpack_decoder_decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers);

#endif //TWO_HPACK_DECODER_H
