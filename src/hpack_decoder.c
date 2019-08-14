#include "hpack_decoder.h"
#include "hpack_utils.h"
#include "hpack_huffman.h"
#include <stdint.h>            /* for int8_t, int32_t*/
#include <string.h>            /* for memset, NULL*/
#include "logging.h"           /* for ERROR */

/*
 * Function: hpack_decoder_decode_integer
 * Decodes an integer using prefix bits for first byte
 * Input:
 *      -> *bytes: Bytes storing encoded integer
 *      -> prefix: size of prefix used to encode integer
 * Output:
 *      returns the decoded integer if succesful, -1 otherwise
 */
uint32_t hpack_decoder_decode_integer(uint8_t *bytes, uint8_t prefix)
{
    int pointer = 0;
    uint8_t b0 = bytes[0];

    b0 = b0 << (8 - prefix);
    b0 = b0 >> (8 - prefix);
    uint8_t p = 255;
    p = p << (8 - prefix);
    p = p >> (8 - prefix);
    if (b0 != p) {
        return (uint32_t)b0;
    }
    else {
        uint32_t integer = (uint32_t)p;
        uint32_t depth = 0;
        for (uint32_t i = 1;; i++) {
            uint8_t bi = bytes[pointer + i];
            if (!(bi & (uint8_t)128)) {
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
                return integer;
            }
            else {
                bi = bi << 1;
                bi = bi >> 1;
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
            }
            depth = depth + 7;
        }
    }
    return -1;
}

/*
 * Function: hpack_decoder_decode_non_huffman_string
 * Decodes an Array of char not compressed with Huffman Compression
 * Input:
 *      -> *str: Buffer to store encoded array
 *      -> *encoded_string: Buffer containing encoded bytes
 * Output:
 *      return the number of bytes written in str if succesful or -1 otherwise
 */
int hpack_decoder_decode_non_huffman_string(char *str, uint8_t *encoded_string)
{
    uint32_t str_length = hpack_decoder_decode_integer(encoded_string, 7);
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);

    for (uint16_t i = 0; i < str_length; i++) {
        str[i] = (char)encoded_string[str_length_size + i];
    }
    return str_length + str_length_size;
}

#ifdef INCLUDE_HUFFMAN_COMPRESSION
/*
 * Function: hpack_decoder_decode_huffman_word
 * Given the bit position from where to start to read, tries to decode de bits using
 * different lengths (from smallest (5) to largest(30)) and stores the result byte in *str.
 * This function is meant to be used internally by hpack_decoder_decode_huffman_string.
 * Input:
 *      -> *str: Pointer to byte to store the result of the decompression process
 *      -> *encoded_string: Buffer storing compressed representation of string to decompress
 *      -> encoded_string_size: Size (in octets) given by header, this is not the same as the decompressed string size
 *      -> bit_position: Starts to decompress from this bit in the *encoded_string
 * Output:
 *      Stores the result in the first byte of str and returns the size in bits of
 *      the encoded word (e.g. 5 or 6 or 7 ...), if an error occurs the return value is -1.
 */
int32_t hpack_decoder_decode_huffman_word(char *str, uint8_t *encoded_string, uint8_t encoded_string_size, uint16_t bit_position)
{
    huffman_encoded_word_t encoded_word;

    for (uint8_t i = 5; i < 31; i++) { //search through all lengths possible

        /*Check if can read buffer*/
        if (bit_position + i > 8 * encoded_string_size) {
            return -2;
        }

        uint32_t result =  hpack_utils_read_bits_from_bytes(bit_position, i, encoded_string);

        encoded_word.code = result;
        encoded_word.length = i;
        uint8_t decoded_sym = 0;

        int8_t rc = hpack_huffman_decode(&encoded_word, &decoded_sym);

        if (rc == 0) {/*Code is found on huffman tree*/
            str[0] = (char)decoded_sym;
            return i;
        }
    }
    ERROR("Couldn't read bits in hpack_decoder_decode_huffman_word");
    return -2;
}
#endif


#ifdef INCLUDE_HUFFMAN_COMPRESSION
/*
 * Function: hpack_decoder_decode_huffman_string
 * Tries to decode encoded_string (compressed using huffman) and store the result in *str
 * Input:
 *      -> *str: Buffer to store the result of the decompression process, this buffer must be bigger than encoded_string
 *      -> *encoded_string: Buffer containing a string compressed using Huffman Compression
 * Output:
 *      Stores the decompressed version of encoded_string in str if successful and returns the number of bytes read
 *      of encoded_string, if it fails to decode the string the return value is -1
 */
int hpack_decoder_decode_huffman_string(char *str, uint8_t *encoded_string)
{
    uint32_t str_length = hpack_decoder_decode_integer(encoded_string, 7);
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
    uint8_t *encoded_buffer = encoded_string + str_length_size;
    uint16_t bit_position = 0;
    uint16_t i = 0;

    for (i = 0; (bit_position - 1) / 8 < (int32_t)str_length; i++) {
        int32_t word_length = hpack_decoder_decode_huffman_word(str + i, encoded_buffer, str_length, bit_position);
        if (word_length < 0) {
            uint8_t bits_left = 8 * str_length - bit_position;
            uint8_t last_byte = encoded_buffer[str_length - 1];

            if (bits_left < 8) {
                uint8_t mask = (1 << bits_left) - 1; /*padding of encoding*/
                if ((last_byte & mask) == mask) {
                    return str_length + str_length_size;
                }
                else {
                    ERROR("Error while trying to decode padding in hpack_decoder_decode_huffman_string");
                    return -2;
                }
            }
            else {
                /*Check if it has a padding greater than 7 bits*/
                uint8_t mask = 255u;
                if (last_byte == mask) {
                    ERROR("Decoding error: The compressed header has a padding greater than 7 bits");
                    return -1;
                }
                DEBUG("Couldn't decode string in hpack_decoder_decode_huffman_string");
                return -2;
            }
        }
        bit_position += word_length;
    }
    return str_length + str_length_size;
}
#endif

/*
 * Function: hpack_decoder_decode_string
 * Decodes an string according to its huffman bit, if it's 1 the string is decoded using huffman decompression
 * if it's 0 it copies the content of the encoded_buffer to str.
 * Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_buffer: Buffer containing the string to decode
 * Output:
 *      Returns the number of bytes read from encoded_buffer in the decoding process if succesful,
 *      if the process fails the function returns -1
 */
uint32_t hpack_decoder_decode_string(char *str, uint8_t *encoded_buffer)
{
    //decode huffman name
    //decode name length
    uint8_t huffman_bit = 128u & *(encoded_buffer);

    if (huffman_bit) {
        #ifdef INCLUDE_HUFFMAN_COMPRESSION
        return hpack_decoder_decode_huffman_string(str, encoded_buffer);
        #else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return -1;
        #endif
    }
    else {
        return hpack_decoder_decode_non_huffman_string(str, encoded_buffer);
    }
}

int hpack_decoder_decode_indexed_header_field(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));

    int8_t rc = hpack_tables_find_entry_name_and_value(dynamic_table, index, name, value);
    if (rc < 0 ) {
        DEBUG("Error en find_entry ");
        return rc;
    }

    pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));
    return pointer;
}

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
int hpack_decoder_decode_literal_header_field_with_incremental_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_with_incremental_indexing");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0 ) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));
    }
    int32_t rc = hpack_decoder_decode_string(value, header_block + pointer);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_with_incremental_indexing");
        return rc;
    }
    pointer += rc;
    /*Here we add it to the dynamic table*/
    int res = hpack_tables_dynamic_table_add_entry(dynamic_table, name, value);
    if(res < 0){
        DEBUG("Couldn't add to dynamic table");
        return res;
    }

    return pointer;
}
#endif
int hpack_decoder_decode_literal_header_field_without_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_without_indexing");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0 ) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));
    }
    int32_t rc = hpack_decoder_decode_string(value, header_block + pointer);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_without_indexing");
        return rc;
    }
    pointer += rc;
    return pointer;
}


int hpack_decoder_decode_literal_header_field_never_indexed(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0 ) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));
    }
    int32_t rc = hpack_decoder_decode_string(value, header_block + pointer);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
        return rc;
    }
    pointer += rc;
    return pointer;
}

/*
   int decode_dynamic_table_size(uint8_t* header_block){
    //int pointer = 0;
    uint32_t new_table_size = decode_integer(header_block, find_prefix_size(DYNAMIC_TABLE_SIZE_UPDATE));//decode index
    //TODO modify dynamic table size and dynamic table if necessary.
    (void)new_table_size;
    ERROR("Not implemented yet.");
    return -1;
   }
 */

/* Function: hpack_decoder_decode_header
 * decodes a header according to the preamble
 * Input:
 *      -> *dynamic_table: table that could be modified by encoder or decoder, it allocates headers
 *      -> *bytes: Buffer containing data to decode
 *      -> *name: Memory to store decoded name
 *      -> *value: Memory to store decoded value
 * Output:
 *      returns the amount of octets in which the pointer has moved to read all the headers
 *
 */
int hpack_decoder_decode_header(hpack_dynamic_table_t *dynamic_table, uint8_t *bytes, hpack_preamble_t preamble, char *name, char *value)
{
    if (preamble == INDEXED_HEADER_FIELD) {
        DEBUG("Decoding an indexed header field");
        int rc = hpack_decoder_decode_indexed_header_field(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in hpack_decoder_decode_indexed_header_field ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING) {
        DEBUG("Decoding a literal header field without indexing");
        int rc = hpack_decoder_decode_literal_header_field_without_indexing(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in hpack_decoder_decode_literal_header_field_without_indexing ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        DEBUG("Decoding a literal header field never indexed");
        int rc = hpack_decoder_decode_literal_header_field_never_indexed(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in hpack_decoder_decode_literal_header_field_never_indexed ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        DEBUG("Decoding a literal header with incremental indexing");
        #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
        int rc = hpack_decoder_decode_literal_header_field_with_incremental_indexing(dynamic_table, bytes, name, value);
        #else
        int rc = hpack_decoder_decode_literal_header_field_never_indexed(dynamic_table, bytes, name, value);
        #endif
        if (rc < 0) {
            ERROR("Error in hpack_decoder_decode_literal_header_field_with_incremental_indexing ");
        }
        return rc;
    }
    else if (preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        DEBUG("Decoding a dynamic table size update");
        //TODO add function to decode dynamic_table_size
        ERROR("Dynamic_table_size_update not implemented yet");
        return -1;
    }
    else {
        ERROR("Error unknown preamble value: %d", preamble);
        return -1;
    }
}

/*
 * Function: hpack_decoder_decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *header_block: Pointer to a sequence of octets (bytes)
 *      -> header_block_size: Size in bytes of the header block that will be decoded
 *      -> headers: struct that allocates a list of headers (pair name and value)
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int hpack_decoder_decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode_header_block_from_table(NULL, header_block, header_block_size, headers);
}

/*
 * Function: hpack_decoder_decode_header_block_from_table
 * decodes an array of headers using a dynamic_table, as it decodes one, the pointer of the headers
 * moves forward also updates the decoded header list
 * Input:
 *      -> *dynamic_table: table that could be modified by encoder or decoder, it allocates headers
 *      -> *header_block: Pointer to a sequence of octets (bytes)
 *      -> header_block_size: Size in bytes of the header block that will be decoded
 *      -> headers: struct that allocates a list of headers (pair name and value)
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int hpack_decoder_decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    int pointer = 0;

    char tmp_name[16];
    char tmp_value[32];

    while (pointer < header_block_size) {
        memset(tmp_name, 0, 16);
        memset(tmp_value, 0, 32);
        hpack_preamble_t preamble = hpack_utils_get_preamble(header_block[pointer]);
        int rc = hpack_decoder_decode_header(dynamic_table, header_block + pointer, preamble, tmp_name, tmp_value);
        headers_add(headers, tmp_name, tmp_value);

        if (rc < 0) {
            DEBUG("Error in hpack_decoder_decode_header ");
            return rc;
        }

        pointer += rc;
    }
    if (pointer > header_block_size) {
        DEBUG("Error decoding header block ... ");
        return -1;
    }
    return pointer;
}
