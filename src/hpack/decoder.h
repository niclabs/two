#ifndef TWO_HPACK_DECODER_H
#define TWO_HPACK_DECODER_H

#include <stdint.h>             /* for uint8_t */
#include "headers.h"            /* for header_list_t */
#include "hpack/tables.h"       /* for hpack_dynamic_table_t */
#include "hpack/utils.h"        /* for hpack_states*/



int hpack_decoder_decode_header_block(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, int32_t header_block_size, header_list_t *headers);
#endif //TWO_HPACK_DECODER_H
