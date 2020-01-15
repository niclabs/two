#include "hpack.h"

#include "hpack/decoder.h"
#include "hpack/encoder.h"
#include "hpack/tables.h"
#include "config.h"
#define LOG_MODULE LOG_MODULE_HPACK
#include "logging.h"


/*
 * Function: decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *states: hpack_pack main struct
 *      -> *header_block: buffer which contains headers
 *      -> header_block_size: size of the header to be read
 *      -> headers: list of headers which has to be updated after decoding
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int hpack_decode(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, int header_block_size, header_list_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode(dynamic_table, header_block, header_block_size, headers);
}

/*
 * Function: encode
 * Encodes a header field
 * Input:
 *      -> *states: hpack main struct wrapper
 *      -> *name_string: name of the header field to encode
 *      -> *value_string: value of the header field to encode
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *  Return the number of bytes written in encoded_buffer (the size of the encoded string) or -1 if it fails to encode
 */
int hpack_encode(hpack_dynamic_table_t *dynamic_table,
                 header_list_t *headers_out,
                 uint8_t *encoded_buffer,
                 uint32_t buffer_size)
{
    int rc = hpack_encoder_encode(dynamic_table,  headers_out, encoded_buffer, buffer_size);
    DEBUG("RETURN FROM HPACK");
    return rc;
}

/*
 * Function: encode_dynamic_size_update
 * Input:
 *      -> *states: hpack main struct wrapper
 *      -> max_size: Max size of the dynamic table
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *      Return the number of bytes written in encoded_buffer
 */
int encode_dynamic_size_update(hpack_dynamic_table_t *dynamic_table, uint32_t max_size, uint8_t *encoded_buffer)
{
    return hpack_encoder_encode_dynamic_size_update(dynamic_table, max_size, encoded_buffer);
}

/*
 * Function: hpack_init_states
 * Initialize the given hpack_states
 * Input:
 *      -> *states: hpack main struct wrapper to initialize
 *      -> settings_max_table_size: max table size set in settings frame
 * Output:
 *      (void)
 */
void hpack_init(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_table_size)
{
    #if HPACK_INCLUDE_DYNAMIC_TABLE
    hpack_tables_init_dynamic_table(dynamic_table, settings_max_table_size);
    #else
    (void)settings_max_table_size;
    (void)dynamic_table;
    #endif
}

void hpack_dynamic_change_max_size(hpack_dynamic_table_t *dynamic_table, uint32_t incoming_max_table_size){
    #if HPACK_INCLUDE_DYNAMIC_TABLE
        if(incoming_max_table_size <= HPACK_MAX_DYNAMIC_TABLE_SIZE){
          dynamic_table->settings_max_table_size = incoming_max_table_size;
          hpack_tables_dynamic_table_resize(dynamic_table, incoming_max_table_size);
        }
    #else
    (void)incoming_max_table_size;
    (void)dynamic_table;
    #endif
}
