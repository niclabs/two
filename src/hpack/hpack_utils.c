#include <stdint.h>     /* for int8_t */
#include "hpack/utils.h"
#include "config.h"
#define LOG_MODULE LOG_MODULE_HPACK
#include "logging.h"

/*
 * Function: hpack_utils_log128
 * Compute the log128 of the input
 * Input:
 *      -> x: variable to apply log128
 * Output:
 *      returns log128(x)
 */
uint32_t hpack_utils_log128(uint32_t x)
{
    uint32_t n = 0;
    uint32_t m = 1;

    while (m < x) {
        m = 1u << (7u * (++n));
    }

    if (m == x) {
        return n;
    }
    return n - 1;
}

/*
 * Function: hpack_utils_get_preamble
 * Matches a numeric preamble to a hpack_preamble_t
 * Input:
 *      -> preamble: Number representing the preamble of the integer to encode
 * Output:
 *      An hpack_preamble_t if it can parse the given preamble or -1 if it fails
 */
hpack_preamble_t hpack_utils_get_preamble(uint8_t preamble)
{
    if (preamble & INDEXED_HEADER_FIELD) {
        return INDEXED_HEADER_FIELD;
    }
    if (preamble & LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    }
    if (preamble & DYNAMIC_TABLE_SIZE_UPDATE) {
        return DYNAMIC_TABLE_SIZE_UPDATE;
    }
    if (preamble & LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        return LITERAL_HEADER_FIELD_NEVER_INDEXED;
    }
    if (preamble < 16) {
        return LITERAL_HEADER_FIELD_WITHOUT_INDEXING; // preamble = 0000
    }
    ERROR("wrong preamble");
    return -1;
}

/*
 * Function: hpack_utils_find_prefix_size
 * Given the preamble octet returns the size of the prefix
 * Input:
 *      -> octet: Preamble of encoding
 * Output:
 *      returns the size in bits of the prefix.
 */
uint8_t hpack_utils_find_prefix_size(hpack_preamble_t octet)
{
    if ((INDEXED_HEADER_FIELD & octet) == INDEXED_HEADER_FIELD) {
        return (uint8_t)7;
    }
    if ((LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING & octet) == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return (uint8_t)6;
    }
    if ((DYNAMIC_TABLE_SIZE_UPDATE & octet) == DYNAMIC_TABLE_SIZE_UPDATE) {
        return (uint8_t)5;
    }
    return (uint8_t)4; /*LITERAL_HEADER_FIELD_WITHOUT_INDEXING and LITERAL_HEADER_FIELD_NEVER_INDEXED*/
}

/* Function: hpack_utils_encoded_integer_size
 * Input:
 *      -> num: Number to encode
 *      -> prefix: Size of prefix
 * Output:
 *      returns the amount of octets used to encode num
 */
uint32_t hpack_utils_encoded_integer_size(uint32_t num, uint8_t prefix)
{
    uint8_t p = (uint8_t)((1u << prefix) - 1u);

    if (num < p) {
        return 1;
    }
    else if (num == p) {
        return 2;
    }
    else {
        uint32_t k = hpack_utils_log128(num - p);//log(num - p) / log(128);
        return k + 2;
    }
}
