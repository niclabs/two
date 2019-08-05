#include "hpack.h"

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

   int compress_static(char* headers, int headers_size, uint8_t* compressed_headers){
    (void)headers;
    (void)headers_size;
    (void)compressed_headers;
    ERROR("compress_static: not implemented yet!");
    return -1;
   }
   int compress_dynamic(char* headers, int headers_size, uint8_t* compressed_headers){
    (void)headers;
    (void)headers_size;
    (void)compressed_headers;
    ERROR("compress_dynamic: not implemented yet!");
    return -1;
   }
 */


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
int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode_header_block_from_table(dynamic_table, header_block, header_block_size, headers);
}
/*
 * Function: decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *header_block: //TODO
 *      -> header_block_size: //TODO
 *      -> headers: //TODO
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode_header_block(header_block, header_block_size, headers);
}
/*
 * Function: encode
 * Encodes a header field
 * Input:
 *      -> preamble: Indicates the type to encode
 *      -> max_size: Max size of the dynamic table
 *      -> index: Index to encode if not equal to 0, if equal to 0 encodes a literal header field representation
 *      -> *name_string: name of the header field to encode
 *      -> name_huffman_bool: Boolean value used to indicate if the name_string is to be compressed using Huffman Compression
 *      -> *value_string: value of the header field to encode
 *      -> value_huffman_bool: Boolean value used to indicate if the value_string is to be compressed using Huffman Compression
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *  Return the number of bytes written in encoded_buffer (the size of the encoded string) or -1 if it fails to encode
 */
int encode(hpack_preamble_t preamble, uint32_t max_size, char *name_string, char *value_string,  uint8_t *encoded_buffer);
{
    return hpack_encoder_encode( preamble, max_size, name_string, value_string,  encoded_buffer);

}
