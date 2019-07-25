#include "hpack.h"
#include "logging.h"

#include "hpack_utils.h"
#include "hpack_huffman.h"
#include "table.h"
#include "headers.h"
#include <stdio.h>
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
//Table related functions and definitions

const uint32_t FIRST_INDEX_DYNAMIC = 62; // Changed type to remove warnings

//HeaderPairs in static table

const char name_0[] = " : authority ";
const char name_1[] = " : method ";
const char name_2[] = " : method ";
const char name_3[] = " : path ";
const char name_4[] = " : path ";
const char name_5[] = " : scheme ";

const char name_6[] = " : scheme ";
const char name_7[] = " : status ";
const char name_8[] = " : status ";
const char name_9[] = " : status ";
const char name_10[] = " : status ";

const char name_11[] = " : status ";
const char name_12[] = " : status ";
const char name_13[] = " : status ";
const char name_14[] = "accept - charset ";
const char name_15[] = "accept - encoding ";

const char name_16[] = "accept - language ";
const char name_17[] = "accept - ranges ";
const char name_18[] = "accept ";
const char name_19[] = "access - control - allow - origin ";
const char name_20[] = "age ";

const char name_21[] = "allow ";
const char name_22[] = "authorization ";
const char name_23[] = "cache - control ";
const char name_24[] = "content - disposition ";
const char name_25[] = "content - encoding ";

const char name_26[] = "content - language ";
const char name_27[] = "content - length ";
const char name_28[] = "content - location ";
const char name_29[] = "content - range ";
const char name_30[] = "content - type ";

const char name_31[] = "cookie ";
const char name_32[] = "date ";
const char name_33[] = "etag ";
const char name_34[] = "expect ";
const char name_35[] = "expires ";

const char name_36[] = "from ";
const char name_37[] = "host ";
const char name_38[] = "if - match ";
const char name_39[] = "if - modified - since ";
const char name_40[] = "if - none - match ";

const char name_41[] = "if - range ";
const char name_42[] = "if - unmodified - since ";
const char name_43[] = "last - modified ";
const char name_44[] = "link ";
const char name_45[] = "location ";

const char name_46[] = "max - forwards ";
const char name_47[] = "proxy - authenticate ";
const char name_48[] = "proxy - authorization ";
const char name_49[] = "range ";
const char name_50[] = "referer ";

const char name_51[] = "refresh ";
const char name_52[] = "retry - after ";
const char name_53[] = "server ";
const char name_54[] = "set - cookie ";
const char name_55[] = "strict - transport - security ";

const char name_56[] = "transfer - encoding ";
const char name_57[] = "user - agent ";
const char name_58[] = "vary ";
const char name_59[] = "via ";
const char name_60[] = "www - authenticate ";
//static table header values

const char value_0[] = " ";
const char value_1[] = "GET ";
const char value_2[] = "POST ";
const char value_3[] = " / ";
const char value_4[] = " / index.html ";
const char value_5[] = "http ";

const char value_6[] = "https ";
const char value_7[] = " 200 ";
const char value_8[] = " 204 ";
const char value_9[] = " 206 ";
const char value_10[] = " 304 ";

const char value_11[] = " 400 ";
const char value_12[] = " 404 ";
const char value_13[] = " 500 ";
const char value_14[] = " ";
const char value_15[] = "gzip, deflate ";

const char value_16[] = " ";
const char value_17[] = " ";
const char value_18[] = " ";
const char value_19[] = " ";
const char value_20[] = " ";

const char value_21[] = " ";
const char value_22[] = " ";
const char value_23[] = " ";
const char value_24[] = " ";
const char value_25[] = " ";

const char value_26[] = " ";
const char value_27[] = " ";
const char value_28[] = " ";
const char value_29[] = " ";
const char value_30[] = " ";

const char value_31[] = " ";
const char value_32[] = " ";
const char value_33[] = " ";
const char value_34[] = " ";
const char value_35[] = " ";

const char value_36[] = " ";
const char value_37[] = " ";
const char value_38[] = " ";
const char value_39[] = " ";
const char value_40[] = " ";

const char value_41[] = " ";
const char value_42[] = " ";
const char value_43[] = " ";
const char value_44[] = " ";
const char value_45[] = " ";

const char value_46[] = " ";
const char value_47[] = " ";
const char value_48[] = " ";
const char value_49[] = " ";
const char value_50[] = " ";

const char value_51[] = " ";
const char value_52[] = " ";
const char value_53[] = " ";
const char value_54[] = " ";
const char value_55[] = " ";

const char value_56[] = " ";
const char value_57[] = " ";
const char value_58[] = " ";
const char value_59[] = " ";
const char value_60[] = " ";

// Then set up a table to refer to your strings.

const char *const static_header_name_table[] = { name_0, name_1, name_2, name_3, name_4, name_5, name_6, name_7, name_8, name_9, name_10, name_11, name_12, name_13, name_14, name_15, name_16, name_17, name_18, name_19, name_20, name_21, name_22, name_23, name_24, name_25, name_26, name_27, name_28, name_29, name_30, name_31, name_32, name_33, name_34, name_35, name_36, name_37, name_38, name_39, name_40, name_41, name_42, name_43, name_44, name_45, name_46, name_47, name_48, name_49, name_50, name_51, name_52, name_53, name_54, name_55, name_56, name_57, name_58, name_59, name_60 };

const char *const static_header_value_table[] = { value_0, value_1, value_2, value_3, value_4, value_5, value_6, value_7, value_8, value_9, value_10, value_11, value_12, value_13, value_14, value_15, value_16, value_17, value_18, value_19, value_20, value_21, value_22, value_23, value_24, value_25, value_26, value_27, value_28, value_29, value_30, value_31, value_32, value_33, value_34, value_35, value_36, value_37, value_38, value_39, value_40, value_41, value_42, value_43, value_44, value_45, value_46, value_47, value_48, value_49, value_50, value_51, value_52, value_53, value_54, value_55, value_56, value_57, value_58, value_59, value_60 };


/*
 * Function: find_entry
 * finds an entry (pair name-value) in either the static or dynamic table_length
 * Input:
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 *      -> *value: buffer where the value of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int find_entry(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);

/*
 * Function: pack_encoded_words_to_bytes
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
int8_t pack_encoded_words_to_bytes(huffman_encoded_word_t *encoded_words, uint8_t encoded_words_size, uint8_t *buffer, uint8_t buffer_size)
{
    int32_t sum = 0;

    for (int i = 0; i < encoded_words_size; i++) {
        sum += encoded_words[i].length;
    }

    uint8_t required_bytes = sum % 8 ? (sum / 8) + 1 : sum / 8;

    if (required_bytes > buffer_size) {
        ERROR("Buffer size is less than the required amount in pack_encoded_words_to_bytes");
        return -1;
    }

    int cur = 0;
    int byte_offset = 0;
    int bit_offset = 0;

    for (int i = 0; i < encoded_words_size; i++) {
        huffman_encoded_word_t word = encoded_words[i];
        uint32_t code = word.code;
        uint8_t code_length = word.length;
        code <<= (32 - code_length);
        while (code_length) {
            uint8_t bits_to_write = 8 - bit_offset > code_length ? code_length : (8 - bit_offset);
            uint32_t mask = (1 << bits_to_write) - 1;
            mask <<= (32 - bits_to_write);
            uint32_t bits = mask & code;
            bits >>= (32 - (8 - bit_offset));
            buffer[byte_offset] |= bits;
            code <<= bits_to_write;
            code_length -= bits_to_write;
            cur += bits_to_write;
            byte_offset = cur / 8;
            bit_offset = cur - 8 * byte_offset;
        }
    }
    if (bit_offset > 0) {
        uint8_t padding = (1 << (8 - bit_offset)) - 1;
        buffer[byte_offset] |= padding;
    }
    return 0;
}

/*
 * Function: read_bits_from_bytes
 * Reads bits from a buffer of bytes (max number of bits it can read is 32).
 * Input:
 * -> current_bit_pointer: The bit from where to start reading (inclusive)
 * -> number_of_bits_to_read: The number of bits to read from the buffer
 * -> *buffer: The buffer containing the bits to read
 * -> buffer_size: Size of the buffer
 * -> *result: Pointer to variable to store the result
 * output: 0 if the bits are read correctly and stores it in *result; -1 if it fails
 */
int8_t read_bits_from_bytes(uint16_t current_bit_pointer, uint8_t number_of_bits_to_read, uint8_t *buffer, uint8_t buffer_size, uint32_t *result)
{
    uint32_t byte_offset = current_bit_pointer / 8;
    uint8_t bit_offset = current_bit_pointer - 8 * byte_offset;
    uint8_t num_bytes = ((number_of_bits_to_read + current_bit_pointer - 1) / 8) - (current_bit_pointer / 8) + 1;

    if (num_bytes + byte_offset > buffer_size) {
        return -1;
    }
    uint32_t mask = 1 << (8 * num_bytes - number_of_bits_to_read);
    mask -= 1;
    mask = ~mask;
    mask >>= bit_offset;
    mask &= ((1 << (8 * num_bytes - bit_offset)) - 1);
    uint32_t code = buffer[byte_offset];
    for (int i = 1; i < num_bytes; i++) {
        code <<= 8;
        code |= buffer[i + byte_offset];
    }
    code &= mask;
    code >>= (8 * num_bytes - number_of_bits_to_read - bit_offset);
    *result = code;
    return 0;
}

/*
 * Function: log128
 * Compute the log128 of the input
 * Input:
 *      -> x: variable to apply log128
 * Output:
 *      returns log128(x)
 */
int log128(uint32_t x)
{
    uint32_t n = 0;
    uint32_t m = 1;

    while (m < x) {
        m = 1 << (7 * (++n));
    }

    if (m == x) {
        return n;
    }
    return n - 1;
}



/* Function: encoded_integer_size
 * Input:
 *      -> num: Number to encode
 *      -> prefix: Size of prefix
 * Output:
 *      returns the amount of octets used to encode num
 */
uint32_t encoded_integer_size(uint32_t num, uint8_t prefix)
{
    uint8_t p = 255;

    p = p << (8 - prefix);
    p = p >> (8 - prefix);
    if (num == p) {
        return 2;
    }
    if (num >= p) {
        uint32_t k = log128(num - p);//log(num - p) / log(128);
        return k + 2;
    }
    else {
        return 1;
    }
}


/*
 * Function: encode_integer
 * encode an integer with the given prefix
 * and save the encoded integer in "encoded_integer"
 * encoded_integer must be an array of the size calculated by encoded_integer_size
 * Input:
 *      -> integer: number to encode
 *      -> prefix:  prefix size
 *      -> *encoded_integer: location to store the result of the encoding
 * Output:
 *      returns the encoded_integer_size
 */
int encode_integer(uint32_t integer, uint8_t prefix, uint8_t *encoded_integer)
{
    int octets_size = 0;
    uint32_t max_first_octet = (1 << prefix) - 1;

    if (integer < max_first_octet) {
        encoded_integer[0] = (uint8_t)(integer << (8 - prefix));
        encoded_integer[0] = (uint8_t)encoded_integer[0] >> (8 - prefix);
        octets_size = 1;
    }
    else {
        uint8_t b0 = 255;
        b0 = b0 << (8 - prefix);
        b0 = b0 >> (8 - prefix);
        integer = integer - b0;
        if (integer == 0) {
            octets_size = 2;
        }
        else {
            uint32_t k = log128(integer);//log(integer)/log(128);
            octets_size = k + 2;
        }

        encoded_integer[0] = b0;

        int i = 1;
        while (integer >= 128) {
            uint32_t encoded = integer % 128;
            encoded += 128;

            encoded_integer[i] = (uint8_t)encoded;
            i++;
            integer = integer / 128;
        }
        uint8_t bi = (uint8_t)integer & 0xff;
        encoded_integer[i] = bi;
    }
    return octets_size;
}

/*
 * Function: decode_integer
 * Decodes an integer using prefix bits for first byte
 * Input:
 *      -> *bytes: Bytes storing encoded integer
 *      -> prefix: size of prefix used to encode integer
 * Output:
 *      returns the decoded integer if succesful, -1 otherwise
 */
uint32_t decode_integer(uint8_t *bytes, uint8_t prefix)
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
 * Function: encode_non_huffman_string
 * Encodes an Array of char without using Huffman Compression
 * Input:
 *      -> *str: Array to encode
 *      -> *encoded_string: Buffer to store the result of the encoding process
 * Output:
 *      returns the size in octets(bytes) of the encoded string
 */
int encode_non_huffman_string(char *str, uint8_t *encoded_string)
{
    int str_length = strlen(str);
    int encoded_string_length_size = encode_integer(str_length, 7, encoded_string); //encode integer(string size) with prefix 7. this puts the encoded string size in encoded string

    for (int i = 0; i < str_length; i++) {                                          //TODO check if strlen is ok to use here
        encoded_string[i + encoded_string_length_size] = str[i];
    }
    return str_length + encoded_string_length_size;
}

/*
 * Function: decode_non_huffman_string
 * Decodes an Array of char not compressed with Huffman Compression
 * Input:
 *      -> *str: Buffer to store encoded array
 *      -> *encoded_string: Buffer containing encoded bytes
 * Output:
 *      return the number of bytes written in str if succesful or -1 otherwise
 */
int decode_non_huffman_string(char *str, uint8_t *encoded_string)
{
    uint32_t str_length = decode_integer(encoded_string, 7);
    uint32_t str_length_size = encoded_integer_size(str_length, 7);

    for (uint16_t i = 0; i < str_length; i++) {
        str[i] = (char)encoded_string[str_length_size + i];
    }
    return str_length;
}

/*
 * Function: encode_huffman_word
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
uint32_t encode_huffman_word(char *str, int str_length, huffman_encoded_word_t *encoded_words)
{
    uint32_t encoded_word_bit_length = 0;

    for (uint16_t i = 0; i < str_length; i++) {
        hpack_huffman_encode(&encoded_words[i], (uint8_t)str[i]);
        encoded_word_bit_length += encoded_words[i].length;
    }
    return encoded_word_bit_length;
}

/*
 * Function: encode_huffman_string
 * Encodes an Array of char using huffman tree compression
 * and stores the result in the given buffer.
 * Input:
 *      -> *str: Array to encode
 *      -> *encoded_string: Buffer to store the result of the compression process.
 * Output:
 *      returns the size (in bytes) of the compressed array.
 */
int encode_huffman_string(char *str, uint8_t *encoded_string)
{
    uint32_t str_length = strlen(str); //TODO check if strlen is ok to use here
    huffman_encoded_word_t encoded_words[str_length];

    uint32_t encoded_word_bit_length = encode_huffman_word(str, str_length, encoded_words);

    uint8_t encoded_word_byte_length = encoded_word_bit_length % 8 ? (encoded_word_bit_length / 8) + 1 : (encoded_word_bit_length / 8);

    uint8_t encoded_buffer[encoded_word_byte_length];

    memset(encoded_buffer, 0, encoded_word_byte_length);

    pack_encoded_words_to_bytes(encoded_words, str_length, encoded_buffer, encoded_word_byte_length);

    int encoded_word_length_size = encode_integer(encoded_word_byte_length, 7, encoded_string);

    /*Set huffman bool*/
    encoded_string[0] |= 128;

    if (encoded_word_byte_length + encoded_word_length_size >= HTTP2_MAX_HBF_BUFFER) {
        ERROR("word too big, does not fit on the buffer_encoded");
        return -1;
    }

    for (int i = 0; i < encoded_word_byte_length; i++) {
        encoded_string[i + encoded_word_length_size] = encoded_buffer[i];
    }

    return encoded_word_byte_length + encoded_word_length_size;
}

/*
 * Function: decode_huffman_word
 * Given the bit position from where to start to read, tries to decode de bits using
 * different lengths (from smallest (5) to largest(30)) and stores the result byte in *str.
 * This function is meant to be used internally by decode_huffman_string.
 * Input:
 *      -> *str: Pointer to byte to store the result of the decompression process
 *      -> *encoded_string: Buffer storing compressed representation of string to decompress
 *      -> encoded_string_size: Size (in octets) given by header, this is not the same as the decompressed string size
 *      -> bit_position: Starts to decompress from this bit in the *encoded_string
 * Output:
 *      Stores the result in the first byte of str and returns the size in bits of
 *      the encoded word (e.g. 5 or 6 or 7 ...), if an error occurs the return value is -1.
 */
int32_t decode_huffman_word(char *str, uint8_t *encoded_string, uint8_t encoded_string_size, uint16_t bit_position)
{
    huffman_encoded_word_t encoded_word;

    for (uint8_t i = 5; i < 31; i++) { //search through all lengths possible
        if (bit_position + i > 8 * encoded_string_size) {
            return -1;
        }
        uint32_t result = 0;
        int8_t rc = read_bits_from_bytes(bit_position, i, encoded_string, encoded_string_size, &result);
        if (rc < 0) {
            ERROR("Error while trying to read bits from encoded_string in decode_huffman_word");
            return -1;
        }
        encoded_word.code = result;
        encoded_word.length = i;
        uint8_t decoded_sym = 0;

        rc = hpack_huffman_decode(&encoded_word, &decoded_sym);

        if (rc == 0) {/*Code is found on huffman tree*/
            str[0] = (char)decoded_sym;
            return i;
        }
    }
    return -1;
}

/*
 * Function: decode_huffman_string
 * Tries to decode encoded_string (compressed using huffman) and store the result in *str
 * Input:
 *      -> *str: Buffer to store the result of the decompression process, this buffer must be bigger than encoded_string
 *      -> *encoded_string: Buffer containing a string compressed using Huffman Compression
 * Output:
 *      Stores the decompressed version of encoded_string in str if successful and returns the number of bytes read
 *      of encoded_string, if it fails to decode the string the return value is -1
 */
int decode_huffman_string(char *str, uint8_t *encoded_string)
{
    uint32_t str_length = decode_integer(encoded_string, 7);
    uint32_t str_length_size = encoded_integer_size(str_length, 7);
    uint8_t *encoded_buffer = encoded_string + str_length_size;
    uint16_t bit_position = 0;
    uint16_t i = 0;

    for (i = 0; (bit_position - 1) / 8 < (int32_t)str_length; i++) {
        int32_t word_length = decode_huffman_word(str + i, encoded_buffer, str_length, bit_position);
        if (word_length < 0) {
            if (8 * str_length - bit_position < 8) {
                uint8_t bits_left = 8 * str_length - bit_position;
                uint8_t mask = (1 << bits_left) - 1; /*padding of encoding*/
                uint8_t last_byte = encoded_buffer[str_length - 1];
                if ((last_byte & mask) == mask) {
                    return str_length;
                }
                else {
                    ERROR("Error while trying to decode padding in decode_huffman_string");
                    return -1;
                }
            }
            else {
                ERROR("Couldn't decode string in decode_huffman_string");
                return -1;
            }
        }
        bit_position += word_length;
    }
    return str_length;
}

/*
 * Function: decode_string
 * Decodes an string according to its huffman bit, if it's 1 the string is decoded using huffman decompression
 * if it's 0 it copies the content of the encoded_buffer to str.
 * Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_buffer: Buffer containing the string to decode
 * Output:
 *      Returns the number of bytes read from encoded_buffer in the decoding process if succesful,
 *      if the process fails the function returns -1
 */
uint32_t decode_string(char *str, uint8_t *encoded_buffer)
{
    //decode huffman name
    //decode name length
    uint8_t huffman_bit = 128u & *(encoded_buffer);
    int rc = 0;

    if (huffman_bit) {
        rc = decode_huffman_string(str, encoded_buffer);
        if (rc < 0) {
            return -1;
        }
    }
    else {
        rc = decode_non_huffman_string(str, encoded_buffer);
        if (rc < 0) {
            return -1;
        }
    }
    return rc;
}

/*
   int encode_string(char* str, uint8_t huffman, uint8_t* encoded_string){
    if(huffman){
        return encode_huffman_string(str, encoded_string);
    }else{
        return encode_non_huffman_string(str,encoded_string);
    }
   };
 */

/*
 * Function: find_prefix_size
 * Given the preamble octet returns the size of the prefix
 * Input:
 *      -> octet: Preamble of encoding
 * Output:
 *      returns the size in bits of the prefix.
 */
uint8_t find_prefix_size(hpack_preamble_t octet)
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


/*
   int encode_literal_ḧeader_field_with_incremental_indexing_indexed_name(uint32_t index, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    int encoding_size = encode_integer(index,find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING),encoded_buffer);//returns size of encoding
    if(encoding_size == -1){
        ERROR("error encoding index");
        return -1;
    }
    encode_buffer[0] |=  LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    pointer += encoding_size; //increase pointer to end of index
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;
   }

   int encode_literal_ḧeader_field_with_incremental_indexing_new_name(char* name_string, uint8_t name_huffman_bool, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    encode_buffer[0] =  LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    pointer += 1; //increase pointer to end of index

    if(name_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, name_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,name_string,encoded_buffer+pointer);
    }
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;
   }
   int encode_literal_ḧeader_field_without_indexing_indexed_name(uint32_t index, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    int encoding_size = encode_integer(index,find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING),encoded_buffer); //returns size of encoding
    if(encoding_size == -1){
        ERROR("error encoding index");
        return -1;
    }
    encode_buffer[0] |=  LITERAL_HEADER_FIELD_WITHOUT_INDEXING;
    pointer += encoding_size; //increase pointer to end of index
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;

   }
   int encode_literal_ḧeader_field_without_indexing_new_name(char* name_string, uint8_t name_huffman_bool, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    encode_buffer[0] =  LITERAL_HEADER_FIELD_WITHOUT_INDEXING;
    pointer += 1; //increase pointer to end of index

    if(name_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, name_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,name_string,encoded_buffer+pointer);
    }
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;
   }
   int encode_literal_ḧeader_field_never_indexed_indexed_name(uint32_t index, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    int encoding_size = encode_integer(index,find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED),encoded_buffer); //returns size of encoding
    if(encoding_size == -1){
        ERROR("error encoding index");
        return -1;
    }
    encode_buffer[0] |=  LITERAL_HEADER_FIELD_NEVER_INDEXED;
    pointer += encoding_size; //increase pointer to end of index
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;
   }
   int encode_literal_header_field_never_indexed_new_name(char* name_string, uint8_t name_huffman_bool, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;
    encode_buffer[0] =  LITERAL_HEADER_FIELD_NEVER_INDEXED;
    pointer += 1; //increase pointer to end of index

    if(name_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, name_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,name_string,encoded_buffer+pointer);
    }
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(index, value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(index,value_string,encoded_buffer+pointer);
    }
    return pointer;
   }
 */


int encode_literal_header_field_new_name( char *name_string, uint8_t name_huffman_bool, char *value_string, uint8_t value_huffman_bool, uint8_t *encoded_buffer)
{
    int pointer = 0;

    if (name_huffman_bool != 0) {
        pointer += encode_huffman_string(name_string, encoded_buffer + pointer);
    }
    else {
        pointer += encode_non_huffman_string(name_string, encoded_buffer + pointer);
    }
    if (value_huffman_bool != 0) {
        pointer += encode_huffman_string(value_string, encoded_buffer + pointer);
    }
    else {
        pointer += encode_non_huffman_string(value_string, encoded_buffer + pointer);
    }
    return pointer;
}
/*
   int encode_literal_header_field_indexed_name( char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;

    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }
    return pointer;
   }*/

int encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index, char *value_string, uint8_t value_huffman_bool, char *name_string, uint8_t name_huffman_bool, uint8_t *encoded_buffer)
{
    if (preamble == DYNAMIC_TABLE_SIZE_UPDATE) { // dynamicTableSizeUpdate
        int encoded_max_size_length = encode_integer(max_size, 5, encoded_buffer);
        encoded_buffer[0] |= preamble;
        return encoded_max_size_length;
    }
    else {
        uint8_t prefix = find_prefix_size(preamble);
        int pointer = 0;
        pointer += encode_integer(index, prefix, encoded_buffer + pointer);
        encoded_buffer[0] |= preamble;                      //set first bit
        if (preamble == (uint8_t)INDEXED_HEADER_FIELD) {    /*indexed header field representation in static or dynamic table*/
            //TODO not implemented yet
            ERROR("Not implemented yet!");
            return pointer;
        }
        else {
            if (index == (uint8_t)0) {
                pointer += encode_literal_header_field_new_name(name_string, name_huffman_bool, value_string, value_huffman_bool, encoded_buffer + pointer);
            }
            else {
                //TOD0
                //pointer += encode_literal_header_field_indexed_name( value_string, value_huffman_bool, encoded_buffer + pointer);
            }
            return pointer;
        }
    }
}


/*int decode(uint8_t *encoded_buffer){

   }*/

/*
 * Function: get_preamble
 * Matches a numeric preamble to a hpack_preamble_t
 * Input:
 *      -> preamble: Number representing the preamble of the integer to encode
 * Output:
 *      An hpack_preamble_t if it can parse the given preamble or -1 if it fails
 */
hpack_preamble_t get_preamble(uint8_t preamble)
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

int decode_indexed_header_field(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(INDEXED_HEADER_FIELD));

    if (find_entry(dynamic_table, index, name, value) == -1) {
        ERROR("Error en find_entry");
        return -1;
    }
    pointer += encoded_integer_size(index, find_prefix_size(INDEXED_HEADER_FIELD));
    return pointer;
}

int decode_literal_header_field_with_incremental_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = decode_string(name, header_block + pointer);
        if (rc < 0) {
            ERROR("Error while trying to decode string in decode_literal_header_field_with_incremental_indexing");
            return -1;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        if (find_entry(dynamic_table, index, name, value) == -1) {
            ERROR("Error en find_entry");
            return -1;
        }
        pointer += encoded_integer_size(index, find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));
    }
    int32_t rc = decode_string(value, header_block + pointer);
    if (rc < 0) {
        ERROR("Error while trying to decode string in decode_literal_header_field_with_incremental_indexing");
        return -1;
    }
    pointer += rc;
    //TODO add to dynamic table
    return pointer;
}

int decode_literal_header_field_without_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = decode_string(name, header_block + pointer);
        if (rc < 0) {
            ERROR("Error while trying to decode string in decode_literal_header_field_without_indexing");
            return -1;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        if (find_entry(dynamic_table, index, name, value) == -1) {
            ERROR("Error en find_entry ");
            return -1;
        }
        pointer += encoded_integer_size(index, find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));
    }
    int32_t rc = decode_string(value, header_block + pointer);
    if (rc < 0) {
        ERROR("Error while trying to decode string in decode_literal_header_field_without_indexing");
        return -1;
    }
    pointer += rc;
    return pointer;
}


int decode_literal_header_field_never_indexed(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index

    if (index == 0) {
        pointer += 1;
        int32_t rc = decode_string(name, header_block + pointer);
        if (rc < 0) {
            ERROR("Error while trying to decode string in decode_literal_header_field_never_indexed");
            return -1;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        int rc = find_entry(dynamic_table, index, name, value);
        if (rc == -1) {
            ERROR("Error en find_entry ");
            return -1;
        }
        pointer += encoded_integer_size(index, find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));
    }
    int32_t rc = decode_string(value, header_block + pointer);
    if (rc < 0) {
        ERROR("Error while trying to decode string in decode_literal_header_field_never_indexed");
        return -1;
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

/* Function: decode_header
 * decodes a header according to the preamble
 * Input:
 *      -> *bytes: Buffer containing data to decode
 *      -> *name: Memory to store decoded name
 *      -> *value: Memory to store decoded value
 * Output:
 *      returns the amount of octets in which the pointer has moved to read all the headers
 *
 */
int decode_header(hpack_dynamic_table_t *dynamic_table, uint8_t *bytes, hpack_preamble_t preamble, char *name, char *value)
{
    if (preamble == INDEXED_HEADER_FIELD) {
        int rc = decode_indexed_header_field(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_without_indexing ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING) {
        int rc = decode_literal_header_field_without_indexing(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_without_indexing ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        int rc = decode_literal_header_field_never_indexed(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_never_indexed ");
        }
        return rc;
    }
    else if (preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        int rc = decode_literal_header_field_never_indexed(dynamic_table, bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_never_indexed ");
        }
        return rc;
    }
    else {
        ERROR("Error unknown preamble value");
        return -1;
    }
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
    return decode_header_block_from_table(NULL, header_block, header_block_size, headers);
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
int decode_header_block_from_table(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)//header_t* h_list, uint8_t * header_counter)
{
    int pointer = 0;

    int headers_decoded = 0;

    while (pointer < header_block_size) {
        hpack_preamble_t preamble = get_preamble(header_block[pointer]);
        int rc = decode_header(dynamic_table, header_block + pointer, preamble, headers->headers[headers->count].name,
                               headers->headers[headers->count].value);

        if (rc < 0) {
            ERROR("Error in decode_header ");
            return -1;
        }

        pointer += rc;
        headers_decoded += 1;
    }
    headers->count += headers_decoded;
    if (pointer > header_block_size) {
        ERROR("Error decoding header block ... ");
        return -1;
    }
    return pointer;
}


/*
 * Function: header_pair_size
 * Input:
 *      -> h: //TODO change the name of this variable
 * Output:
 *      return the size of a header HeaderPair, the size is
 *      the sum of octets used for its encoding and 32
 */
uint32_t header_pair_size(header_pair_t h)
{
    return (uint32_t)(strlen(h.name) + strlen(h.value) + 32);
}

/*
 * Function: hpack_init_dynamic_table
 * //TODO
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> dynamic_table_max_size: //TODO
 * Output:
 *      //TODO
 */
int hpack_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size)
{
    dynamic_table->max_size = dynamic_table_max_size;
    dynamic_table->table_length = (uint32_t)((dynamic_table->max_size / 32) + 1);
    dynamic_table->first = 0;
    dynamic_table->next = 0;
    header_pair_t table[dynamic_table->table_length];
    dynamic_table->table = table;
    return 0;
}

/*
 * Function: dynamic_table_length
 * Input:
 *      -> *dynamic_table: //TODO
 * Output:
 *      returns the actual table length which is equal to the number of entries in the table_length
 */
uint32_t dynamic_table_length(hpack_dynamic_table_t *dynamic_table)
{
    uint32_t table_length_used = dynamic_table->first < dynamic_table->next ?
                                 (dynamic_table->next - dynamic_table->first) % dynamic_table->table_length :
                                 (dynamic_table->table_length - dynamic_table->first + dynamic_table->next) % dynamic_table->table_length;

    return table_length_used;
}

/*
 * Function: dynamic_table_size
 * Input:
 *      -> *dynamic_table: //TODO
 * Output:
 *      returns the size of the table, this is the sum of each header pair's size
 */
uint32_t dynamic_table_size(hpack_dynamic_table_t *dynamic_table)
{
    uint32_t total_size = 0;
    uint32_t table_length = dynamic_table_length(dynamic_table);

    for (uint32_t i = 0; i < table_length; i++) {
        total_size += header_pair_size(dynamic_table->table[(i + dynamic_table->first) % table_length]);
    }
    return total_size;
}

/*
 * Function: dynamic_table_add_entry
 * Add an header pair entry in the table
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> *name: //TODO
 *      -> *value: //TODO
 * Output:
 *      //TODO
 */
//header pair is a name string and a value string
int dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    uint32_t entry_size = (uint32_t)(strlen(name) + strlen(value) + 32);

    if (entry_size > dynamic_table->max_size) {
        ERROR("New entry size exceeds the size of table ");
        return -1; //entry's size exceeds the size of table
    }

    while (entry_size + dynamic_table_size(dynamic_table) > dynamic_table->max_size) {
        dynamic_table->first = (dynamic_table->first + 1) % dynamic_table->table_length;
    }

    dynamic_table->table[dynamic_table->next].name = name;
    dynamic_table->table[dynamic_table->next].value = value;
    dynamic_table->next = (dynamic_table->next + 1) % dynamic_table->table_length;
    return 0;
}

/*
 * Function: dynamic_table_resize
 * Makes an update of the size of the dynamic table_length
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> new_max_size: //TODO
 *      -> dynamic_table_max_size: //TODO
 * Output:
 *      return 0 if the update is succesful, or -1 otherwise
 */
int dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t new_max_size, uint32_t dynamic_table_max_size)
{
    if (new_max_size > dynamic_table_max_size) {
        ERROR("Resize operation exceeds the maximum size set by the protocol ");
        return -1;
    }

    uint32_t new_table_length = (uint32_t)(new_max_size / 32) + 1;
    header_pair_t new_table[new_table_length];

    uint32_t new_first = 0;
    uint32_t new_next = 0;
    uint32_t new_size = 0;

    uint32_t i = dynamic_table->first;
    uint32_t j = 0;

    uint32_t table_length_used = dynamic_table_length(dynamic_table);

    while (j < table_length_used && (new_size + header_pair_size(dynamic_table->table[i])) <= new_max_size) {
        new_table[j] = dynamic_table->table[i];
        i = (i + 1) % dynamic_table->table_length;
        j++;
        new_next = (new_next + 1) % new_table_length;
        new_size += header_pair_size(dynamic_table->table[i]);
    }

    //clean last table_length
    memset(&dynamic_table->table, 0, sizeof(dynamic_table->table));

    dynamic_table->max_size = new_max_size;
    dynamic_table->table_length = new_table_length;
    dynamic_table->next = new_next;
    dynamic_table->first = new_first;
    dynamic_table->table = new_table;


    return 0;

}



/*
 * Function: dynamic_find_entry
 * Finds entry in dynamic table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> index: //TODO
 * Output:
 *       //TODO
 */
header_pair_t dynamic_find_entry(hpack_dynamic_table_t *dynamic_table, uint32_t index)
{
    uint32_t table_index = (dynamic_table->next + dynamic_table->table_length - (index - 61)) % dynamic_table->table_length;

    return dynamic_table->table[table_index];
}

/*
 * Function: find_entry
 * General method to an entry in the table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: //TODO
 *      -> index: //TODO
 *      -> *name: //TODO
 *      -> *value: //TODO
 * Output:
 *      return -1 in case of error, returns 0 and copy the entry values into name and value buffers
 */
int find_entry(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value)
{
    const char *table_name; //add const before char to resolve compilation warnings
    const char *table_value;

    if (index >= FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }
        header_pair_t entry = dynamic_find_entry(dynamic_table, index);
        table_name = entry.name;
        table_value = entry.value;
    }
    else {
        index--; //because static table begins at index 1
        table_name = static_header_name_table[index];
        table_value = static_header_value_table[index];
    }
    strncpy(name, table_name, strlen(table_name));
    strncpy(value, table_value, strlen(table_value));
    return 0;

}
