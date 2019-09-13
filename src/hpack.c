#include "hpack.h"
#include "hpack_decoder.h"
#include "hpack_encoder.h"
#include "hpack_tables.h"

/*
 *
 * returns compressed headers size or -1 if error
 */
/*
   int compress_huffman(char* headers, int headers_size, uint8_t* compressed_headers){
    (void)headers;
    (void)headers_size;
    (void)compressed_headers;
    ERROR("compress_huffman: not implemented yet!");
    return -1;
   }

/*
 * Function: decode_header_block_from_table
 * decodes an array of headers using a dynamic_table, as it decodes one, the pointer of the headers
 * moves forward also updates the decoded header list
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> *header_block: //TODO
 *      -> header_block_size: //TODO
 *      -> headers: //TODO
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
/*
   int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
   {
    return hpack_decoder_decode_header_block_from_table(dynamic_table, header_block, header_block_size, headers);
   }*/
/*
 * Function: decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *states: hpack_pack main struct
 *      -> *header_block: //TODO
 *      -> header_block_size: //TODO
 *      -> headers: //TODO
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int decode_header_block(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode_header_block(states, header_block, header_block_size, headers);
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
int encode(hpack_states_t *states, char *name_string, char *value_string,  uint8_t *encoded_buffer)
{
    return hpack_encoder_encode(states, name_string, value_string, encoded_buffer);
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
int encode_dynamic_size_update(hpack_states_t *states, uint32_t max_size, uint8_t *encoded_buffer)
{
    return hpack_encoder_encode_dynamic_size_update(states, max_size, encoded_buffer);
}

/*
 * Function: hpack_init_states
 * Initialize the given hpack_states
 * Input:
 *      -> *states: hpack main struct wrapper to intitialize
 * Output:
 *      returns 0 if successful
 */
int8_t hpack_init_states(hpack_states_t *states, uint32_t settings_max_table_size)
{

    int8_t rc_dynamic = hpack_tables_init_dynamic_table(&states->dynamic_table, settings_max_table_size);

    if (rc_dynamic < 0) {
        return rc_dynamic;
    }
    states->settings_max_table_size = settings_max_table_size;
    return 0;
}
