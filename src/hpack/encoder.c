#include <stdint.h>             /*for int8_t, int32_t  */
#include <string.h>             /* for strlen, memset    */

#include "hpack/huffman.h"      /* for huffman_encoded_word_t, hpack_huffman_...*/
#include "hpack/encoder.h"
#include "hpack/utils.h"

#include "config.h"
#define LOG_MODULE LOG_MODULE_HPACK
#include "logging.h"

#if (INCLUDE_HUFFMAN_COMPRESSION)

/*
 * Function: hpack_encoder_pack_encoded_words_to_bytes
 * Writes bits from 'code' (the representation in huffman)
 * from an array of huffman_encoded_word_t to a buffer
 * using exactly the sum of all lengths of encoded words bits.
 * Input:
 * - *encoded_words: Array of encoded_words to compress
 * - encoded_words_size: Size of encoded_words array
 * - *buffer: The buffer to store the result of the compression
 * - buffer_size: Size of the buffer
 * output: 0 if the bits are stored correctly ; -1 if it fails because the size of the buffer is less than the required to store the result
 */
int8_t hpack_encoder_pack_encoded_words_to_bytes(huffman_encoded_word_t *encoded_words, uint32_t encoded_words_size,
                                                 uint8_t *buffer, uint32_t buffer_size)
{
    uint32_t sum = 0;

    for (uint32_t i = 0; i < encoded_words_size; i++) {
        sum += encoded_words[i].length;
    }

    uint32_t required_bytes = (sum % 8) ? (sum / 8) + 1 : sum / 8;

    if (required_bytes > buffer_size) {
        ERROR("Buffer size is less than the required amount in hpack_encoder_pack_encoded_words_to_bytes");
        return INTERNAL_ERROR;
    }

    uint32_t cur = 0;
    uint32_t byte_offset = 0;
    uint32_t bit_offset = 0;

    for (uint32_t i = 0; i < encoded_words_size; i++) {
        huffman_encoded_word_t word = encoded_words[i];
        uint32_t code = word.code;
        uint8_t code_length = word.length;
        code <<= (32u - code_length);
        while (code_length) {
            uint32_t bits_to_write = 8 - bit_offset > code_length ? code_length : (8 - bit_offset);
            uint32_t mask = (1u << bits_to_write) - 1u;
            mask <<= (32u - bits_to_write);
            uint32_t bits = mask & code;
            bits >>= (32u - (8u - bit_offset));
            buffer[byte_offset] |= bits;
            code <<= bits_to_write;
            code_length -= bits_to_write;
            cur += bits_to_write;
            byte_offset = cur / 8u;
            bit_offset = cur - 8u * byte_offset;
        }
    }
    if (bit_offset > 0) {
        uint8_t padding = (uint8_t)((1u << (8u - bit_offset)) - 1u);
        buffer[byte_offset] |= padding;
    }
    return 0;
}

#endif

/*
 * Function: hpack_encoder_encode_integer
 * encode an integer with the given prefix
 * and save the encoded integer in "encoded_integer"
 * encoded_integer must be an array of the size calculated by encoded_integer_size
 * Input:
 *      -> integer: number to encode
 *      -> prefix:  prefix size
 *      -> *encoded_integer: location to store the result of the encoding
 * Output:
 *      returns the encoded_integer_size, or -2 if it fails
 */
int hpack_encoder_encode_integer(uint32_t integer, uint8_t prefix, uint8_t *encoded_integer)
{
    if (integer > HPACK_MAXIMUM_INTEGER) {
        return -1;
    }
    int octets_size = hpack_utils_encoded_integer_size(integer, prefix);
    uint8_t max_first_octet = (uint8_t)((1u << prefix) - 1u);

    if (integer < max_first_octet) {
        encoded_integer[0] = (uint8_t)(integer << (8u - prefix));
        encoded_integer[0] = encoded_integer[0] >> (8u - prefix);
    }
    else {
        uint8_t b0 = (uint8_t)((1u << prefix) - 1u);
        integer = integer - b0;
        encoded_integer[0] = b0;
        int i = 1;

        while (integer >= 128) {
            uint32_t encoded = integer % 128;
            encoded += 128;

            encoded_integer[i] = (uint8_t)encoded;
            i++;
            integer = integer / 128;
        }

        uint8_t bi = (uint8_t)(integer & 0xffu);
        encoded_integer[i] = bi;
    }
    return octets_size;
}

/*
 * Function: hpack_encoder_encode_non_huffman_string
 * Encodes an Array of char without using Huffman Compression
 * Input:
 *      -> *str: Array to encode
 *      -> *encoded_string: Buffer to store the result of the encoding process
 * Output:
 *      returns the size in octets(bytes) of the encoded string
 */
int hpack_encoder_encode_non_huffman_string(char *str, uint8_t *encoded_string, uint32_t buffer_size)
{
    uint32_t str_length = (uint32_t)strlen(str);
    int encoded_string_length_size = hpack_encoder_encode_integer(str_length, 7,
                                                                  encoded_string); //encode integer(string size) with prefix 7. this puts the encoded string size in encoded string

    if (encoded_string_length_size < 0) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }

    if (str_length + encoded_string_length_size >= buffer_size) {
        DEBUG("String too big, does not fit on the encoded_string");
        return -2;
    }
    for (uint32_t i = 0; i < str_length; i++) {                                 //TODO check if strlen is ok to use here
        encoded_string[i + encoded_string_length_size] = (uint8_t)str[i];
    }
    return (int)str_length + encoded_string_length_size;
}

#if (INCLUDE_HUFFMAN_COMPRESSION)

/*
 * Function: hpack_encoder_encode_huffman_word
 * Encodes an Array of char using huffman tree compression
 * and stores the result in an Array of huffman_encoded_word_t.
 * This function is meant to be used with encode_huffman_string.
 * Input:
 *      -> *str: Array to encode
 *      -> str_length: Size of the array to encode
 *      -> *encoded_words: Array to store the result of the encoding process
 * Output:
 *      Returns the sum of all bit lengths of the encoded_words
 */
uint32_t hpack_encoder_encode_huffman_word(char *str, uint32_t str_length, huffman_encoded_word_t *encoded_words)
{
    uint32_t encoded_word_bit_length = 0;

    for (uint32_t i = 0; i < str_length; i++) {
        hpack_huffman_encode(&encoded_words[i], (uint8_t)str[i]);
        encoded_word_bit_length += encoded_words[i].length;
    }
    return encoded_word_bit_length;
}

#endif

#if (INCLUDE_HUFFMAN_COMPRESSION)
/*
 * Function: hpack_encoder_encode_huffman_string
 * Encodes an Array of char using huffman tree compression
 * and stores the result in the given buffer.
 * Input:
 *      -> *str: Array to encode
 *      -> *encoded_string: Buffer to store the result of the compression process.
 * Output:
 *      returns the size (in bytes) of the compressed array.
 */
int hpack_encoder_encode_huffman_string(char *str, uint8_t *encoded_string, uint32_t buffer_size)
{
    uint32_t str_length = (uint32_t)strlen(str);  //TODO check if strlen is ok to use here
    huffman_encoded_word_t encoded_words[str_length];

    uint32_t encoded_word_bit_length = hpack_encoder_encode_huffman_word(str, str_length, encoded_words);

    uint32_t encoded_word_byte_length = (encoded_word_bit_length % 8) ?
                                        (encoded_word_bit_length / 8) + 1 :
                                        (encoded_word_bit_length / 8);
    int encoded_word_length_size = hpack_encoder_encode_integer(encoded_word_byte_length, 7, encoded_string);

    if (encoded_word_length_size < 0) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }

    if (encoded_word_byte_length + encoded_word_length_size >= buffer_size) {
        DEBUG("String too big, does not fit on the encoded_string");
        return -2;
    }

    uint8_t encoded_buffer[encoded_word_byte_length];
    memset(encoded_buffer, 0, encoded_word_byte_length);

    hpack_encoder_pack_encoded_words_to_bytes(encoded_words, str_length, encoded_buffer, encoded_word_byte_length);

    /*Set huffman bool*/
    encoded_string[0] |= 128u;

    for (uint32_t i = 0; i < encoded_word_byte_length; i++) {
        encoded_string[i + encoded_word_length_size] = encoded_buffer[i];
    }

    return (int)encoded_word_byte_length + encoded_word_length_size;
}

#endif

/*
 * Function: hpack_encoder_encode_string
 * Encodes the given string with or without Huffman Compression and stores the result in encoded_string
 * The string is compressed if the string size in octets is less than the uncompressed string
 * Input:
 *      -> *str: Buffer containing string to encode
 *      -> *encoded_string: Buffer to store the encoded
 * Output:
 *      Return the number of bytes used to store the encoded string, or if the encoding fails it returns -1
 */
int hpack_encoder_encode_string(char *str, uint8_t *encoded_string, uint32_t buffer_size)
{
#if (INCLUDE_HUFFMAN_COMPRESSION)
    int rc = hpack_encoder_encode_huffman_string(str, encoded_string, buffer_size);

    if (rc < 0 || (uint32_t)rc > strlen(str)) {
        return hpack_encoder_encode_non_huffman_string(str, encoded_string, buffer_size);
    }
    return rc;
#else
    return hpack_encoder_encode_non_huffman_string(str, encoded_string, buffer_size);
#endif
}


/*
 * Function: hpack_encoder_pack_header
 * Prepares an hpack_encoded_header to send it, it maintains all the logic of chosing which type of compression to use
 * Input:
 *      -> *states: Pointer to hpack_states struct
 *      -> *name_string: string of the name of the entry to pack
 *      -> *value_string: string of the value of the entry to pack
 * Output:
 *
 */
void hpack_encoder_pack_header(hpack_encoded_header_t *encoded_header, hpack_dynamic_table_t *dynamic_table,
                               char *name_string, char *value_string)
{

    //first search if the name-value entry is already in the dynamic table
    int index = hpack_tables_find_index(dynamic_table, name_string, value_string);

    //if index exists
    if (index > 0) {
        //make indexed header field
        DEBUG("Encoding an indexed header field");
        encoded_header->preamble = INDEXED_HEADER_FIELD;
        encoded_header->index = (uint32_t)index;
    }
    else {
        //check if the name of entry is already in the dynamic table
        index = hpack_tables_find_index_name(dynamic_table, name_string);

        if (index < 0) {
            //entry doesn't exist in the dynamic table
            encoded_header->index = 0;
        }
        else {
            encoded_header->index = (uint32_t)index;
        }

#if HPACK_INCLUDE_DYNAMIC_TABLE
        int8_t added = hpack_tables_dynamic_table_add_entry(dynamic_table, name_string, value_string);
        if (added < 0) {
            DEBUG("Couldn't add to dynamic table");
            encoded_header->preamble = LITERAL_HEADER_FIELD_NEVER_INDEXED;
            DEBUG("Encoding a literal header field never indexed");
        }
        else {
            //Successfully added to the dynamic table
            DEBUG("Encoding a literal header field with incremental indexing");
            encoded_header->preamble = LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
        }
#else
        //No dynamic table, encode the string withouth index
        encoded_header->preamble = LITERAL_HEADER_FIELD_NEVER_INDEXED;
        DEBUG("Encoding a literal header field never indexed");
#endif
    }
}

/*
 * Function: hpack_encoder_encode_header
 * Encodes an already prepared header into the encoded buffer to send
 * Input:
 *      -> *encoded_header: Pointer to hpack_encoded_header struct to encode
 *      -> *name_string: string of the name of the entry to pack
 *      -> *value_string: string of the value of the entry to pack
 *      -> *encoded_buffer: buffer to encode and send
 * Output:
 *      Returns the number of bytes written in encoded_buffer
 */
int hpack_encoder_encode_header(hpack_encoded_header_t *encoded_header,
                                char *name_string,
                                char *value_string,
                                uint8_t *encoded_buffer,
                                uint32_t buffer_size)
{

    int pointer = 0;
    uint8_t prefix = hpack_utils_find_prefix_size(encoded_header->preamble);
    int rc;

    //initializes first byte
    encoded_buffer[0] = 0;

    if (encoded_header->index == 0) {
        pointer += 1;
        //try to encode name
        rc = hpack_encoder_encode_string(name_string, encoded_buffer + pointer, buffer_size);
        if (rc < 0) {
            ERROR("Error while trying to encode name string");
            return rc;
        }
        pointer += rc;
        //try to encode value
        rc = hpack_encoder_encode_string(value_string, encoded_buffer + pointer, buffer_size);
        if (rc < 0) {
            ERROR("Error while trying to encode value string");
            return rc;
        }
        pointer += rc;
    }
    else {
        //entry name is already indexed
        rc = hpack_encoder_encode_integer(encoded_header->index, prefix, encoded_buffer + pointer);
        if (rc < 0) {
            ERROR("Integer exceeds implementation limits");
            return PROTOCOL_ERROR; //TODO: Check this return of function hpack_pack_header
        }
        pointer += rc;

        //If header is indexed_header field, nothing else is left to be done
        if (encoded_header->preamble != INDEXED_HEADER_FIELD) {
            //the value has to be written
            rc = hpack_encoder_encode_string(value_string, encoded_buffer + pointer, buffer_size);
            if (rc < 0) {
                ERROR("Error while trying to encode value string");
                return rc;
            }
            pointer += rc;
        }
    }
    //attach the preamble to the first byte of the buffer
    encoded_buffer[0] |= encoded_header->preamble;
    return pointer;
}


/*
 * Function: hpack_encoder_encode
 * Encodes a header field
 * Input:
 *      -> *states: Hpack states of the encoder
 *      -> *name_string: name of the header field to encode
 *      -> *value_string: value of the header field to encode
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *  Return the number of bytes written in encoded_buffer (the size of the encoded string) or -1 if it fails to encode
 */
int hpack_encoder_encode(hpack_dynamic_table_t *dynamic_table,
                         header_list_t *headers_out,
                         uint8_t *encoded_buffer,
                         uint32_t buffer_size)
{
    int pointer = 0;

    header_t headers_array[headers_count(headers_out)];

    headers_get_all(headers_out, headers_array);

    for (int32_t i = 0; i < headers_count(headers_out); i++) {
//
        //this gets the info required to encode into the encoded_header, also decides which header type to use
        hpack_encoded_header_t encoded_header = { 0 };

        hpack_encoder_pack_header(&encoded_header, dynamic_table, headers_array[i].name, headers_array[i].value);
        //finally encode header into buffer
        int rc = hpack_encoder_encode_header(&encoded_header,
                                             headers_array[i].name,
                                             headers_array[i].value,
                                             encoded_buffer + pointer,
                                             buffer_size);

        pointer += rc;
    }
    return pointer;
}


/*
 * Function: hpack_encoder_encode_dynamic_size_update
 * Encodes a dynamic table size update with the new max_size as max_size
 * Input:
 *      -> *dynamic_table: Dynamic table of the encoder
 *      -> max_size: New size of the dynamic table
 *      -> *encoded_buffer: Buffer to encode the max_size
 * Output:
 *      Returns the size in bytes of the update, or an int < 0 if an error occurs.
 */
int hpack_encoder_encode_dynamic_size_update(hpack_dynamic_table_t *dynamic_table, uint32_t max_size, uint8_t *encoded_buffer)
{
    #if HPACK_INCLUDE_DYNAMIC_TABLE
    DEBUG("Encoding a dynamic table size update");
    hpack_preamble_t preamble = DYNAMIC_TABLE_SIZE_UPDATE;
    int8_t rc = hpack_tables_dynamic_table_resize(dynamic_table, max_size);
    if (rc < 0) {
        return rc;
    }
    int encoded_max_size_length = hpack_encoder_encode_integer(max_size, 5, encoded_buffer);
    if (encoded_max_size_length < 0) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }
    encoded_buffer[0] |= preamble;

    return encoded_max_size_length;
#else
    DEBUG("Trying to resize dynamic table while in No dynamic table mode");
    return -2;
#endif
}
