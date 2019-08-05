#include "hpack_encoder.h"

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
    int octets_size = hpack_utils_encoded_integer_size(integer, prefix);
    uint8_t max_first_octet = (1 << prefix) - 1;

    if (integer < max_first_octet) {
        encoded_integer[0] = (uint8_t)(integer << (8 - prefix));
        encoded_integer[0] = (uint8_t)encoded_integer[0] >> (8 - prefix);
    }
    else {
        uint8_t b0 = (1 << prefix) - 1;
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
        uint8_t bi = (uint8_t)integer & 0xff;
        encoded_integer[i] = bi;
    }
    return octets_size;
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

    if (str_length + encoded_string_length_size >= HTTP2_MAX_HBF_BUFFER) {
        ERROR("String too big, does not fit on the encoded_string");
        return -1;
    }
    for (int i = 0; i < str_length; i++) {                                          //TODO check if strlen is ok to use here
        encoded_string[i + encoded_string_length_size] = str[i];
    }
    return str_length + encoded_string_length_size;
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
    int encoded_word_length_size = encode_integer(encoded_word_byte_length, 7, encoded_string);

    if (encoded_word_byte_length + encoded_word_length_size >= HTTP2_MAX_HBF_BUFFER) {
        ERROR("String too big, does not fit on the encoded_string");
        return -1;
    }

    uint8_t encoded_buffer[encoded_word_byte_length];
    memset(encoded_buffer, 0, encoded_word_byte_length);

    pack_encoded_words_to_bytes(encoded_words, str_length, encoded_buffer, encoded_word_byte_length);

    /*Set huffman bool*/
    encoded_string[0] |= 128;

    for (int i = 0; i < encoded_word_byte_length; i++) {
        encoded_string[i + encoded_word_length_size] = encoded_buffer[i];
    }

    return encoded_word_byte_length + encoded_word_length_size;
}

/*
 * Function: encode_string
 * Encodes the given string with or without Huffman Compression and stores the result in encoded_string
 * The string is compressed if the string size in octets is less than the uncompressed string
 * Input:
 *      -> *str: Buffer containing string to encode
 *      -> *encoded_string: Buffer to store the encoded
 * Output:
 *      Return the number of bytes used to store the encoded string, or if the encoding fails it returns -1
 */
int encode_string(char *str, uint8_t *encoded_string)
{
    int rc = encode_huffman_string(str, encoded_string);

    if (rc < 0) {
        return encode_non_huffman_string(str, encoded_string);
    }
    if ((uint16_t)rc > strlen(str)) {
        return encode_non_huffman_string(str, encoded_string);
    }
    return rc;
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

/*
 * Function: encode_literal_header_field_new_name
 * Encodes a name and a value
 * Input:
 *      -> *name_string: name of the header field to encode
 *      -> *value_string: value of the header field to encode
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 *  Output:
 *      Returns the number of bytes used to encode name and value, or -1 if an error occurs while encoding
 */
int encode_literal_header_field_new_name(char *name_string, char *value_string, uint8_t *encoded_buffer)
{
    int pointer = 0;

    int rc = encode_string(name_string, encoded_buffer + pointer);

    if (rc < 0) {
        ERROR("Error while trying to encode name in encode_literal_header_field_new_name");
        return -1;
    }
    pointer += rc;
    rc = encode_string(value_string, encoded_buffer + pointer);
    if (rc < 0) {
        ERROR("Error while trying to encode value in encode_literal_header_field_new_name");
        return -1;
    }
    pointer += rc;
    return pointer;
}

/*
 * Function: encode_literal_header_field_indexed_name
 * Encodes a value
 * Input:
 *      -> *value_string: value of the header field to encode
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *      Returns the number of bytes used to encde the value, or -1 if an error occurs while encoding
 */
int encode_literal_header_field_indexed_name(char *value_string, uint8_t *encoded_buffer)
{
    int rc = encode_string(value_string, encoded_buffer);

    if (rc < 0) {
        ERROR("Error while trying to encode value in encode_literal_header_field_indexed_name");
        return -1;
    }
    return rc;
}

/*
 * Function: encode_indexed_header_field
 * Encodes an indexed indexed header field
 * Input:
 *      -> *dynamic_table: Dynamic table to find name and value
 *      -> *name: Buffer containing name of the header
 *      -> *value: Buffer containing value of the header
 * Output:
 *      Returns the number of octets used to encode the given name and value, or -1 if it fails.
 */
int encode_indexed_header_field(hpack_dynamic_table_t *dynamic_table, char *name, char *value, uint8_t *encoded_buffer)
{
    int rc = hpack_tables_find_index(dynamic_table, name, value);

    if (rc < 0) {
        return -1;
    }
    uint8_t prefix = hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD);
    int pointer = encode_integer(rc, prefix, encoded_buffer);
    return pointer;
}

/*
 * Function: hpack_encoder_encode
 * Encodes a header field
 * Input:
 *      -> preamble: Indicates the type to encode
 *      -> max_size: Max size of the dynamic table
 *      -> *name_string: name of the header field to encode
 *      -> *value_string: value of the header field to encode
 *      -> *encoded_buffer: Buffer to store the result of the encoding process
 * Output:
 *  Return the number of bytes written in encoded_buffer (the size of the encoded string) or -1 if it fails to encode
 */
int hpack_encoder_encode(hpack_preamble_t preamble, uint32_t max_size, char *name_string, char *value_string,  uint8_t *encoded_buffer)
{
    if (preamble == DYNAMIC_TABLE_SIZE_UPDATE) { // dynamicTableSizeUpdate
        int encoded_max_size_length = encode_integer(max_size, 5, encoded_buffer);
        encoded_buffer[0] |= preamble;
        return encoded_max_size_length;
    }
    else {
        //TODO use dynamic table
        int index = hpack_tables_find_index(NULL, name_string, value_string);
        int pointer = 0;
        if(index < 0) {
            index = hpack_tables_find_index_name(NULL, name_string);
            preamble = LITERAL_HEADER_FIELD_NEVER_INDEXED; //TODO select indexing depending on input of func
            uint8_t prefix = hpack_utils_find_prefix_size(preamble);
            if(index < 0){
                //NEW NAME
                encoded_buffer[0] = 0; //Set up name not indexed;
                int rc = encode_literal_header_field_new_name(name_string, value_string, encoded_buffer + pointer);
                if (rc < 0) {
                    ERROR("Error while trying to encode");
                    return -1;
                }
                pointer += rc;
            }else{
                //INDEXED NAME
                pointer += encode_integer(index, prefix, encoded_buffer + pointer);
                int rc = encode_literal_header_field_indexed_name(value_string, encoded_buffer + pointer);
                if (rc < 0) {
                    ERROR("Error while trying to encode");
                    return -1;
                }
                pointer += rc;
            }
            encoded_buffer[0] |= preamble;                      //set first bits
            return pointer;
        } else {
            //INDEXED HEADER FIELD
            preamble = INDEXED_HEADER_FIELD;
            int rc = encode_indexed_header_field(NULL, name_string, value_string, encoded_buffer);
            if (rc < 0) {
                ERROR("Error while trying to encode");
                return -1;
            }
            encoded_buffer[0] |= preamble;                      //set first bits
            return rc;
        }
    }
}
