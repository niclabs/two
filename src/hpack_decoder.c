#include "hpack_decoder.h"
#include "hpack_utils.h"
#include "hpack_huffman.h"
#include <stdint.h>             /* for int8_t, int32_t*/
#include <string.h>             /* for memset, NULL*/
#include "logging.h"            /* for ERROR */

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
        #if (b0 > HPACK_MAXIMUM_INTEGER_SIZE)
        return -1;
        #else
        return (uint32_t)b0;
        #endif
    }
    else {
        uint32_t integer = (uint32_t)p;
        uint32_t depth = 0;
        for (uint32_t i = 1;; i++) {
            uint8_t bi = bytes[pointer + i];
            if (!(bi & (uint8_t)128)) {
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
                if (integer > HPACK_MAXIMUM_INTEGER_SIZE) {
                    DEBUG("Integer is %d:", integer);
                    return -1;
                }
                else {
                    return integer;
                }
            }
            else {
                bi = bi << 1;
                bi = bi >> 1;
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
            }
            depth = depth + 7;
        }
    }
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
    int32_t str_length = hpack_decoder_decode_integer(encoded_string, 7);

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
            /*DEBUG("Can't read more bits i:%d ; bit_position: %d ; encoded_string_size: %d",i,bit_position,encoded_string_size);
               for(uint8_t j = 0 ; j <= encoded_string_size; j++){
                DEBUG("Byte %d is: %d",j,encoded_string[j]);
               }*/
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
    int32_t str_length = hpack_decoder_decode_integer(encoded_string, 7);

    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
    uint8_t *encoded_buffer = encoded_string + str_length_size;
    uint16_t bit_position = 0;
    uint16_t i = 0;

    for (i = 0; (bit_position - 1) / 8 < (int16_t)str_length; i++) {
        int32_t word_length = hpack_decoder_decode_huffman_word(str + i, encoded_buffer, str_length, bit_position);
        if (word_length < 0) {
            if (word_length == -1) { /*Compression Error: It's the EOS Symbol*/
                return -1;
            }
            uint8_t bits_left = 8 * str_length - bit_position;
            uint8_t last_byte = encoded_buffer[str_length - 1];

            if (bits_left < 8) {
                uint8_t mask = (1 << bits_left) - 1; /*padding of encoding*/
                if ((last_byte & mask) == mask) {
                    return str_length + str_length_size;
                }
                else {
                    DEBUG("Last byte is %d", last_byte);
                    ERROR("Decoding error: The compressed header padding contains a value different from the EOS symbol");
                    return -1;
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

int32_t hpack_decoder_check_huffman_padding(uint16_t bit_position, uint8_t *encoded_buffer, uint32_t str_length, uint32_t str_length_size)
{
    uint8_t bits_left = 8 * str_length - bit_position;
    uint8_t last_byte = encoded_buffer[str_length - 1];

    if (bits_left < 8) {
        uint8_t mask = (1 << bits_left) - 1; /*padding of encoding*/
        if ((last_byte & mask) == mask) {
            return str_length + str_length_size;
        }
        else {
            DEBUG("Last byte is %d", last_byte);
            ERROR("Decoding error: The compressed header padding contains a value different from the EOS symbol");
            return -1;
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

#ifdef INCLUDE_HUFFMAN_COMPRESSION
int32_t hpack_decoder_decode_huffman_string_v2(char *str, uint8_t *encoded_string, uint32_t str_length)
{
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
    uint16_t bit_position = 0;
    uint16_t i = 0;

    for (i = 0; ((bit_position - 1) / 8) < (int32_t)str_length; i++) {
        int32_t word_length = hpack_decoder_decode_huffman_word(str + i, encoded_string, str_length, bit_position);
        if (word_length < 0) {
            return hpack_decoder_check_huffman_padding(bit_position, encoded_string, str_length, str_length_size);
        }
        bit_position += word_length;
    }
    return str_length + str_length_size;
}
#endif

int32_t hpack_decoder_decode_non_huffman_string_v2(char *str, uint8_t *encoded_string, uint32_t str_length)
{
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
    for (uint16_t i = 0; i < str_length; i++) {
        str[i] = (char)encoded_string[i];
    }
    return str_length + str_length_size;
}

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
int32_t hpack_decoder_decode_string(char *str, uint8_t *encoded_buffer)
{
    //decode huffman name
    //decode name length
    uint8_t huffman_bit = 128u & *(encoded_buffer);

    if (huffman_bit) {
        #ifdef INCLUDE_HUFFMAN_COMPRESSION
        return hpack_decoder_decode_huffman_string(str, encoded_buffer);
        #else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return -2;
        #endif
    }
    else {
        return hpack_decoder_decode_non_huffman_string(str, encoded_buffer);
    }
}

/*
 * Function: hpack_decoder_decode_name_string
 * Decodes a name string according to its huffman bit, if it's 1 the string is decoded using huffman decompression
 * if it's 0 it copies the content of the encoded_buffer to str.
 * Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_header: Encoded header to decode
 * Output:
 *      Returns the number of bytes read from encoded_header in the decoding process if successful,
 *      if the process fails the function returns -2 or -1.
 */
int32_t hpack_decoder_decode_name_string(char *str, hpack_encoded_header_t *encoded_header)
{
    if (encoded_header->huffman_bit_of_name) {
#ifdef INCLUDE_HUFFMAN_COMPRESSION
        return hpack_decoder_decode_huffman_string_v2(str, encoded_header->name_string, encoded_header->name_length);
#else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return -2;
#endif
    }
    else {
        return hpack_decoder_decode_non_huffman_string_v2(str, encoded_header->name_string, encoded_header->name_length);
    }
}

/*
 * Function: hpack_decoder_decode_value_string
 * Decodes a name string according to its huffman bit, if it's 1 the string is decoded using huffman decompression
 * if it's 0 it copies the content of the encoded_buffer to str.
 * Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_header: Encoded header to decode
 * Output:
 *      Returns the number of bytes read from encoded_header in the decoding process if successful,
 *      if the process fails the function returns -2 or -1.
 */
int32_t hpack_decoder_decode_value_string(char *str, hpack_encoded_header_t *encoded_header)
{
    if (encoded_header->huffman_bit_of_value) {
#ifdef INCLUDE_HUFFMAN_COMPRESSION
        return hpack_decoder_decode_huffman_string_v2(str, encoded_header->value_string, encoded_header->value_length);
#else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return -2;
#endif
    }
    else {
        return hpack_decoder_decode_non_huffman_string_v2(str, encoded_header->value_string, encoded_header->value_length);
    }
}


int hpack_decoder_decode_indexed_header_field(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    int32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));

    int8_t rc = hpack_tables_find_entry_name_and_value(dynamic_table, index, name, value);

    if (rc < 0) {
        DEBUG("Error en find_entry %d",rc);
        return rc;
    }

    pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));
    return pointer;
}

int hpack_decoder_decode_indexed_header_field_v2(hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header, char *name, char *value)
{
    int pointer = 0;
    int8_t rc = hpack_tables_find_entry_name_and_value(dynamic_table, encoded_header->index, name, value);

    if (rc < 0) {
        DEBUG("Error en find_entry %d",rc);
        return rc;
    }

    pointer += hpack_utils_encoded_integer_size(encoded_header->index, hpack_utils_find_prefix_size(encoded_header->preamble));
    return pointer;
}


#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
int hpack_decoder_decode_literal_header_field_with_incremental_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    int32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_with_incremental_indexing");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0) {
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
    if (res < 0) {
        DEBUG("Couldn't add to dynamic table");
        return res;
    }

    return pointer;
}
#endif


int hpack_decoder_decode_literal_header_field_without_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    int32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_without_indexing");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0) {
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
    int32_t index = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index

    if (index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc = hpack_decoder_decode_string(name, header_block + pointer);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, index, name);
        if (rc < 0) {
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


int hpack_decoder_decode_literal_header_field(hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header, char *name, char *value)
{
    int pointer = 0;

    /*New name*/
    if (encoded_header->index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc = hpack_decoder_decode_name_string(name, encoded_header);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(dynamic_table, encoded_header->index, name);
        if (rc < 0) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(encoded_header->index, hpack_utils_find_prefix_size(encoded_header->preamble));
    }

    int32_t rc = hpack_decoder_decode_value_string(value, encoded_header);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
        return rc;
    }
    pointer += rc;
    if (encoded_header->preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
        /*Here we add it to the dynamic table*/
        int rc = hpack_tables_dynamic_table_add_entry(dynamic_table, name, value);
        if (rc < 0) {
            DEBUG("Couldn't add to dynamic table");
            return rc;
        }
#else
        ERROR("Dynamic Table is not included, couldn't add header to table");
        return -1;
#endif
    }
    return pointer;
}

/*
 * Function: hpack_decode_dynamic_table_size_update
 * Decodes a header that represents a dynamic table size update
 * Input:
 *      -> *header_block: Header block to decode
 *      -> *dynamic_table: dynamic table to resize
 * Output:
 *      Returns the number of bytes decoded if successful or a value < 0 if an error occurs.
 */
int hpack_decoder_decode_dynamic_table_size_update(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block)
{
    int32_t new_table_size = hpack_decoder_decode_integer(header_block, hpack_utils_find_prefix_size(DYNAMIC_TABLE_SIZE_UPDATE));//decode index

    DEBUG("New table size is %d", new_table_size);
    int8_t rc = hpack_tables_dynamic_table_resize(dynamic_table, new_table_size);
    if (rc < 0) {
        DEBUG("Dynamic table failed to resize");
        return rc;
    }

    return hpack_utils_encoded_integer_size(new_table_size, hpack_utils_find_prefix_size(DYNAMIC_TABLE_SIZE_UPDATE));
}

/*
 * Function: hpack_decode_dynamic_table_size_update_v2
 * Decodes a header that represents a dynamic table size update
 * Input:
 *      -> *header_block: Header block to decode
 *      -> *dynamic_table: dynamic table to resize
 * Output:
 *      Returns the number of bytes decoded if successful or a value < 0 if an error occurs.
 */
int hpack_decoder_decode_dynamic_table_size_update_v2(hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header)
{
    DEBUG("New table size is %d", encoded_header->dynamic_table_size);
    int8_t rc = hpack_tables_dynamic_table_resize(dynamic_table, encoded_header->dynamic_table_size);
    if (rc < 0) {
        DEBUG("Dynamic table failed to resize");
        return rc;
    }
    return hpack_utils_encoded_integer_size(encoded_header->dynamic_table_size, hpack_utils_find_prefix_size(encoded_header->preamble));
}


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
            DEBUG("Error in hpack_decoder_decode_indexed_header_field ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING) {
        DEBUG("Decoding a literal header field without indexing");
        int rc = hpack_decoder_decode_literal_header_field_without_indexing(dynamic_table, bytes, name, value);
        if (rc < 0) {
            DEBUG("Error in hpack_decoder_decode_literal_header_field_without_indexing ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        DEBUG("Decoding a literal header field never indexed");
        int rc = hpack_decoder_decode_literal_header_field_never_indexed(dynamic_table, bytes, name, value);
        if (rc < 0) {
            DEBUG("Error in hpack_decoder_decode_literal_header_field_never_indexed ");
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
            DEBUG("Error in hpack_decoder_decode_literal_header_field_with_incremental_indexing ");
        }
        return rc;
    }
    else if (preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        DEBUG("Decoding a dynamic table size update");
        int rc = hpack_decoder_decode_dynamic_table_size_update(dynamic_table, bytes);
        if (rc < 0) {
            DEBUG("Error in hpack_decoder_decode_dynamic_table_size_update ");
        }
        return rc;
    }
    else {
        ERROR("Error unknown preamble value: %d", preamble);
        return -2;
    }
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
int hpack_decoder_decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)
{
    int pointer = 0;

    char tmp_name[16];
    char tmp_value[32];

    while (pointer < header_block_size) {
        memset(tmp_name, 0, 16);
        memset(tmp_value, 0, 32);
        hpack_preamble_t preamble = hpack_utils_get_preamble(header_block[pointer]);
        int rc = hpack_decoder_decode_header(dynamic_table, header_block + pointer, preamble, tmp_name, tmp_value);
        if (rc < 0) {
            DEBUG("Error in hpack_decoder_decode_header ");
            return rc;
        }
        if (tmp_name[0] != 0 && tmp_value != 0) {
            headers_add(headers, tmp_name, tmp_value);
        }
        pointer += rc;
    }
    if (pointer > header_block_size) {
        DEBUG("Error decoding header block, read %d bytes and header_block_size is %d", pointer, header_block_size);
        return -1;
    }
    return pointer;
}

uint8_t get_huffman_bit(uint8_t num)
{
    return 128u & num;
}

int8_t hpack_decoder_parse_encoded_header(hpack_encoded_header_t *encoded_header, uint8_t *header_block)
{
    int32_t pointer = 0;

    encoded_header->preamble = hpack_utils_get_preamble(header_block[pointer]);
    encoded_header->index = hpack_decoder_decode_integer(&header_block[pointer], hpack_utils_find_prefix_size(encoded_header->preamble));//decode index
    int32_t index_size = hpack_utils_encoded_integer_size(encoded_header->index,  hpack_utils_find_prefix_size(encoded_header->preamble));
    pointer += index_size;

    if (encoded_header->preamble == INDEXED_HEADER_FIELD) {
        return pointer;
    }

    if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        encoded_header->dynamic_table_size = encoded_header->index;
        encoded_header->index = 0;
        return pointer;
    }

    if (encoded_header->index == 0) {
        encoded_header->huffman_bit_of_name = get_huffman_bit(header_block[pointer]);
        int32_t name_length = hpack_decoder_decode_integer(&header_block[pointer], 7);

        /*Integer exceed implementations limits*/
        if (name_length < 0) {
            ERROR("Integer exceeds implementations limits");
            return -1;
        }

        encoded_header->name_length = (uint32_t)name_length;
        pointer += hpack_utils_encoded_integer_size(encoded_header->name_length, 7);
        encoded_header->name_string = &header_block[pointer];
        pointer += encoded_header->name_length;
    }
    encoded_header->huffman_bit_of_value = get_huffman_bit(header_block[pointer]);
    int32_t value_length = hpack_decoder_decode_integer(&header_block[pointer], 7);

    /*Integer exceed implementations limits*/
    if (value_length < 0) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }

    encoded_header->value_length = value_length;
    pointer += hpack_utils_encoded_integer_size(encoded_header->value_length, 7);
    encoded_header->value_string = &header_block[pointer];
    pointer += encoded_header->value_length;
    return pointer;
}

int hpack_decoder_decode_header_v2(hpack_encoded_header_t *encoded_header, hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    if (encoded_header->preamble == INDEXED_HEADER_FIELD) {
        DEBUG("Decoding an indexed header field");
        return hpack_decoder_decode_indexed_header_field_v2(dynamic_table, encoded_header, name, value);
    }
    else if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        DEBUG("Decoding a dynamic table size update");
        return hpack_decoder_decode_dynamic_table_size_update_v2(dynamic_table, encoded_header);
    }
    else if (encoded_header->preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING
             || encoded_header->preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED
             || encoded_header->preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return hpack_decoder_decode_literal_header_field(dynamic_table, encoded_header, name, value);
    }

    else {
        ERROR("Error unknown preamble value: %d", encoded_header->preamble);
        return -2;
    }
}

int hpack_check_eos_symbol(uint8_t *encoded_buffer, uint8_t buffer_length)
{
    uint32_t eos = 0x3fffffff;

    for (uint32_t i = 0; i + 3 < buffer_length; i++) {
        uint32_t symbol = 0;
        symbol |= (encoded_buffer[i]) << 24;
        symbol |= (encoded_buffer[i + 1]) << 16;
        symbol |= (encoded_buffer[i + 2]) << 8;
        symbol |= (encoded_buffer[i + 3]);
        if (symbol == eos) {
            ERROR("Decoding error: The compressed header contains the EOS Symbol");
            return -1;
        }
    }
    return 0;
}

int hpack_decoder_check_errors(hpack_encoded_header_t *encoded_header)
{
    /*Row doesn't exist*/
    if (encoded_header->preamble == INDEXED_HEADER_FIELD && encoded_header->index == 0) {
        ERROR("Decoding Error: Cannot retrieve a 0 index from hpack tables");
        return -1;
    }

    /*Integer exceed implementations limits*/
    if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE && encoded_header->dynamic_table_size > HPACK_MAXIMUM_INTEGER_SIZE) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }

    /*It's ok*/
    if (encoded_header->preamble == INDEXED_HEADER_FIELD || encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        return 0;
    }

    if ((encoded_header->index == 0) && (encoded_header->name_length >= 4)) {
        /*Check if it's the EOS Symbol in name_string*/
        int rc = hpack_check_eos_symbol(encoded_header->name_string, encoded_header->name_length);
        if (rc < 0) {
            return rc;
        }
    }
    if (encoded_header->value_length >= 4) {
        /*Check if it's the EOS Symbol in value_string*/
        int rc = hpack_check_eos_symbol(encoded_header->value_string, encoded_header->value_length);
        if (rc < 0) {
            return rc;
        }
    }
    return 0;
}

int hpack_decoder_decode_header_block_v2(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)
{
    int pointer = 0;

    char tmp_name[16];
    char tmp_value[32];

    while (pointer < header_block_size) {
        hpack_encoded_header_t encoded_header;
        memset(tmp_name, 0, 16);
        memset(tmp_value, 0, 32);
        int bytes_read = hpack_decoder_parse_encoded_header(&encoded_header, header_block + pointer);
        if (bytes_read < 0) {
            /*Error*/
            return bytes_read;
        }
        pointer += bytes_read;
        int err = hpack_decoder_check_errors(&encoded_header);
        if (err < 0) {
            return err;
        }
        int rc = hpack_decoder_decode_header_v2(&encoded_header, dynamic_table, tmp_name, tmp_value);
        if (rc < 0) {
            return rc;
        }
        if (tmp_name[0] != 0 && tmp_value[0] != 0) {
            headers_add(headers, tmp_name, tmp_value);
        }
    }
    if (pointer > header_block_size) {
        DEBUG("Error decoding header block, read %d bytes and header_block_size is %d", pointer, header_block_size);
        return -2;
    }
    return pointer;
}

/*
 * Function: hpack_decoder_decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *dynamic_table: Pointer to dynamic table to store headers
 *      -> *header_block: Pointer to a sequence of octets (bytes)
 *      -> header_block_size: Size in bytes of the header block that will be decoded
 *      -> headers: struct that allocates a list of headers (pair name and value)
 * Output:
 *      returns the amount of octets in which the pointer has move to read all the headers
 */
int hpack_decoder_decode_header_block(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    //return hpack_decoder_decode_header_block_v2(dynamic_table, header_block, header_block_size, headers);

    return hpack_decoder_decode_header_block_from_table(dynamic_table, header_block, header_block_size, headers);
}
