#include "hpack.h"
#include "logging.h"

#include "hpack_utils.h"
#include "table.h"
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

//finds an entry (pair name-value) in either the static or dynamic table_length
//returns -1 in case of Error
int find_entry(uint32_t index, char *name, char *value);

/*
 * Function: Writes bits from 'code' (the representation in huffman)
 * from an array of huffman_encoded_word_t to a buffer
 * using exactly the sum of all lengths of encoded words.
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
    return 0;
}
/*
 * Function: Reads bits from a buffer of bytes (max number of bits it can read is 32).
 * Input:
 * - current_bit_pointer: The bit from where to start reading (inclusive)
 * - number_of_bits_to_read: The number of bits to read from the buffer
 * - *buffer: The buffer containing the bits to read
 * - buffer_size: Size of the buffer
 * - *result: Pointer to variable to store the result
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



/*returns the amount of octets used to encode a int num with a prefix p*/
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
 * encode an integer with the given prefix
 * and save the encoded integer in "encoded_integer"
 * encoded_integer must be an array of the size calculated by encoded_integer_size
 * returns the encoded_integer_size
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
   int encode_huffman_word(char* str, uint8_t* encoded_word){
    (void) str;
    (void) encoded_word;
    ERROR("Not implemented yet");
    return -1;
   }
 */

/*
   int encode_huffman_string(char* str, uint8_t* encoded_string){

    uint8_t encoded_word[HTTP2_MAX_HBF_BUFFER];
    int encoded_word_length = encode_huffman_word(str, encoded_word); //save de encoded word in "encoded_word" and returns the length of the encoded word

    if(encoded_word_length>=HTTP2_MAX_HBF_BUFFER){
        ERROR("word too big, does not fit on the encoded_word_buffer");
        return -1;
    }
    int encoded_word_length_size = encode_integer(encoded_word_length,7, encoded_string);//encodes the length of the encoded word, adn returns the size of the encoding of the length of the encoded word

    encoded_string[0] |= (uint8_t)128;//check this

    int new_size = encoded_word_length+encoded_word_length_size;

    for(int i = 0; i < encoded_word_length; i++){
        encoded_string[i+encoded_word_length_size] = encoded_word[i];
    }
    return new_size;
   };
 */

/*
   int encode_string(char* str, uint8_t huffman, uint8_t* encoded_string){
    if(huffman){
        return encode_huffman_string(str, encoded_string);
    }else{
        return encode_non_huffman_string(str,encoded_string);
    }
   };
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
   int pack_huffman_string_and_size(char* string,uint8_t* encoded_buffer){
    uint8_t encoded_string[HTTP2_MAX_HBF_BUFFER];
    int pointer = 0;
    int encoded_size = encode_huffman_string(string, encoded_string);
    if(encoded_size>=HTTP2_MAX_HBF_BUFFER){
        ERROR("string encoded size too big");
        return -1;
    }
    int encoded_size_size = encode_integer(encoded_size,7,encoded_buffer+pointer);
    encoded_buffer[pointer] |=(uint8_t)128; //set hauffman bit
    pointer += encoded_size_size;
    for(int i = 0; i< encoded_size;i++){
        encoded_buffer[pointer+i]=encoded_string[i];
    }
    pointer += encoded_size;
    return pointer;
   }
 */

int pack_non_huffman_string_and_size(char *string, uint8_t *encoded_buffer)
{
    int pointer = 0;
    int string_size = strlen(string);
    int encoded_size = encode_integer(string_size, 7, encoded_buffer + pointer);

    pointer += encoded_size;
    for (int i = 0; i < string_size; i++) {
        encoded_buffer[pointer + i] = string[i];
    }
    pointer += string_size;
    return pointer;
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
        //TODO
        //pointer += pack_huffman_string_and_size(name_string,encoded_buffer+pointer);
    }
    else {
        pointer += pack_non_huffman_string_and_size(name_string, encoded_buffer + pointer);
    }
    if (value_huffman_bool != 0) {
        //TODO
        //pointer += pack_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }
    else {
        pointer += pack_non_huffman_string_and_size(value_string, encoded_buffer + pointer);
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
                pointer += encode_literal_header_field_new_name( name_string, name_huffman_bool, value_string, value_huffman_bool, encoded_buffer + pointer);
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

/*uint32_t decode_integer(uint8_t* bytes, uint8_t prefix){
    int pointer = 0;
    uint8_t first_byte = (bytes[0]<<(8-prefix))>>(8-prefix);
    uint32_t sum = (pow(2,prefix)-1);
    if(first_byte==(uint8_t)sum){
        pointer +=1;
        while(bytes[pointer]&(uint8_t)128){

        }

    }
    else{
        return (uint32_t)first_byte;
    }
   }
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
   int decode_literal_header_field_incremental_index(uint8_t* header_block, char* name, char* value){
    //int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));//decode index
    if(index == 0){
        //TODO
        //decode huffman name
        //decode name length
        //decode name
        (void)name;
    }
    else{
        //TODO find name in table
    }
    //TODO
    //decode value length
    //decode value
    (void)value;
    //TODO add to dynamic table
    ERROR("Not implemented yet.");
    return -1;
   }*/

int decode_literal_header_field_without_indexing(uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index

    if (index == 0) {
        pointer += 1;
        //decode huffman name
        //decode name length
        int name_length = decode_integer(header_block + pointer, 7);
        pointer += encoded_integer_size(name_length, 7);
        //decode name
        char *rc = strncpy(name, (char *)header_block + pointer, name_length);
        if (rc <= (char *)0) {
            ERROR("Error en strncpy");
            return -1;
        }
        pointer += name_length;
    }
    else {
        //find entry in either static or dynamic table_length
        if (find_entry(index, name, value) == -1) {
            ERROR("Error en find_entry");
            return -1;
        }
        pointer += encoded_integer_size(index, find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));
    }
    //decode value length
    int value_length = decode_integer(header_block + pointer, 7);
    pointer += encoded_integer_size(value_length, 7);
    //decode value
    strncpy(value, (char *)header_block + pointer, value_length);
    pointer += value_length;

    return pointer;
}


int decode_literal_header_field_never_indexed(uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index

    if (index == 0) {
        pointer += 1;
        //decode huffman name
        //decode name length
        int name_length = decode_integer(header_block + pointer, 7);
        pointer += encoded_integer_size(name_length, 7);
        //decode name
        char *rc = strncpy(name, (char *)header_block + pointer, name_length);
        if (rc <= (char *)0) {
            ERROR("Error en strncpy");
            return -1;
        }
        pointer += name_length;
    }
    else {
        //find entry in either static or dynamic table_length
        int rc = find_entry(index, name, value);
        if (rc == -1) {
            ERROR("Error en find_entry");
            return -1;
        }
        pointer += encoded_integer_size(index, find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));
    }
    //decode value length
    int value_length = decode_integer(header_block + pointer, 7);
    pointer += encoded_integer_size(value_length, 7);
    //decode value
    strncpy(value, (char *)header_block + pointer, value_length);
    pointer += value_length;

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

//decodes a header depending on the get_preamble
//returns the amount of octets in which the pointer has move to read all the headers
int decode_header(uint8_t *bytes, hpack_preamble_t preamble, char *name, char *value)
{
    if (preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING) {
        int rc = decode_literal_header_field_without_indexing(bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_without_indexing");
        }
        return rc;
    }
    if (preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED) {
        int rc = decode_literal_header_field_never_indexed(bytes, name, value);
        if (rc < 0) {
            ERROR("Error in decode_literal_header_field_never_indexed");
        }
        return rc;
    }
    else {
        ERROR("Not implemented yet.");
        return -1;
    }
}

//decodes an array of headers,
//as it decodes one, the pointer of the headers move forwards
//also has to update the decoded header lists
//returns the amount of octets in which the pointer has move to read all the headers
int decode_header_block(uint8_t *header_block, uint8_t header_block_size, headers_data_lists_t *h_list)
{
    int pointer = 0;

    int header_counter = 0;

    while (pointer < header_block_size) {
        hpack_preamble_t preamble = get_preamble(header_block[pointer]);
        int rc = decode_header(header_block + pointer, preamble, h_list->header_list_in[h_list->header_list_count_in + header_counter].name,
                               h_list->header_list_in[h_list->header_list_count_in + header_counter].value);

        if (rc < 0) {
            ERROR("Error in decode_header");
            return -1;
        }

        pointer += rc;
        header_counter += 1;
    }
    h_list->header_list_count_in += header_counter;
    if (pointer > header_block_size) {
        ERROR("Error decoding header block...");
        return -1;
    }
    return pointer;
}





//Table related functions and definitions

const uint32_t FIRST_INDEX_DYNAMIC = 62; // Changed type to remove warnings

//HeaderPairs in static table

const char name_0[] = ":authority";
const char name_1[] = ":method";
const char name_2[] = ":method";
const char name_3[] = ":path";
const char name_4[] = ":path";
const char name_5[] = ":scheme";

const char name_6[] = ":scheme";
const char name_7[] = ":status";
const char name_8[] = ":status";
const char name_9[] = ":status";
const char name_10[] = ":status";

const char name_11[] = ":status";
const char name_12[] = ":status";
const char name_13[] = ":status";
const char name_14[] = "accept-charset";
const char name_15[] = "accept-encoding";

const char name_16[] = "accept-language";
const char name_17[] = "accept-ranges";
const char name_18[] = "accept";
const char name_19[] = "access-control-allow-origin";
const char name_20[] = "age";

const char name_21[] = "allow";
const char name_22[] = "authorization";
const char name_23[] = "cache-control";
const char name_24[] = "content-disposition";
const char name_25[] = "content-encoding";

const char name_26[] = "content-language";
const char name_27[] = "content-length";
const char name_28[] = "content-location";
const char name_29[] = "content-range";
const char name_30[] = "content-type";

const char name_31[] = "cookie";
const char name_32[] = "date";
const char name_33[] = "etag";
const char name_34[] = "expect";
const char name_35[] = "expires";

const char name_36[] = "from";
const char name_37[] = "host";
const char name_38[] = "if-match";
const char name_39[] = "if-modified-since";
const char name_40[] = "if-none-match";

const char name_41[] = "if-range";
const char name_42[] = "if-unmodified-since";
const char name_43[] = "last-modified";
const char name_44[] = "link";
const char name_45[] = "location";

const char name_46[] = "max-forwards";
const char name_47[] = "proxy-authenticate";
const char name_48[] = "proxy-authorization";
const char name_49[] = "range";
const char name_50[] = "referer";

const char name_51[] = "refresh";
const char name_52[] = "retry-after";
const char name_53[] = "server";
const char name_54[] = "set-cookie";
const char name_55[] = "strict-transport-security";

const char name_56[] = "transfer-encoding";
const char name_57[] = "user-agent";
const char name_58[] = "vary";
const char name_59[] = "via";
const char name_60[] = "www-authenticate";
//static table header values

const char value_0[] = "";
const char value_1[] = "GET";
const char value_2[] = "POST";
const char value_3[] = "/";
const char value_4[] = "/index.html";
const char value_5[] = "http";

const char value_6[] = "https";
const char value_7[] = "200";
const char value_8[] = "204";
const char value_9[] = "206";
const char value_10[] = "304";

const char value_11[] = "400";
const char value_12[] = "404";
const char value_13[] = "500";
const char value_14[] = "";
const char value_15[] = "gzip, deflate";

const char value_16[] = "";
const char value_17[] = "";
const char value_18[] = "";
const char value_19[] = "";
const char value_20[] = "";

const char value_21[] = "";
const char value_22[] = "";
const char value_23[] = "";
const char value_24[] = "";
const char value_25[] = "";

const char value_26[] = "";
const char value_27[] = "";
const char value_28[] = "";
const char value_29[] = "";
const char value_30[] = "";

const char value_31[] = "";
const char value_32[] = "";
const char value_33[] = "";
const char value_34[] = "";
const char value_35[] = "";

const char value_36[] = "";
const char value_37[] = "";
const char value_38[] = "";
const char value_39[] = "";
const char value_40[] = "";

const char value_41[] = "";
const char value_42[] = "";
const char value_43[] = "";
const char value_44[] = "";
const char value_45[] = "";

const char value_46[] = "";
const char value_47[] = "";
const char value_48[] = "";
const char value_49[] = "";
const char value_50[] = "";

const char value_51[] = "";
const char value_52[] = "";
const char value_53[] = "";
const char value_54[] = "";
const char value_55[] = "";

const char value_56[] = "";
const char value_57[] = "";
const char value_58[] = "";
const char value_59[] = "";
const char value_60[] = "";

// Then set up a table to refer to your strings.

const char *const static_header_name_table[] = { name_0, name_1, name_2, name_3, name_4, name_5, name_6, name_7, name_8, name_9, name_10, name_11, name_12, name_13, name_14, name_15, name_16, name_17, name_18, name_19, name_20, name_21, name_22, name_23, name_24, name_25, name_26, name_27, name_28, name_29, name_30, name_31, name_32, name_33, name_34, name_35, name_36, name_37, name_38, name_39, name_40, name_41, name_42, name_43, name_44, name_45, name_46, name_47, name_48, name_49, name_50, name_51, name_52, name_53, name_54, name_55, name_56, name_57, name_58, name_59, name_60 };

const char *const static_header_value_table[] = { value_0, value_1, value_2, value_3, value_4, value_5, value_6, value_7, value_8, value_9, value_10, value_11, value_12, value_13, value_14, value_15, value_16, value_17, value_18, value_19, value_20, value_21, value_22, value_23, value_24, value_25, value_26, value_27, value_28, value_29, value_30, value_31, value_32, value_33, value_34, value_35, value_36, value_37, value_38, value_39, value_40, value_41, value_42, value_43, value_44, value_45, value_46, value_47, value_48, value_49, value_50, value_51, value_52, value_53, value_54, value_55, value_56, value_57, value_58, value_59, value_60 };


//typedef for HeadedPair
typedef struct hpack_headed_pair {
    char *name;
    char *value;
} headed_pair;

//typedefs for dinamic
//TODO check size of struct
typedef struct hpack_dynamic_table {
    int max_size;
    int first;
    int next;
    int table_length;
    headed_pair **table;
} hpack_dynamic_table;

//dynamic_table functions
//TODO initialize dynamic_table properly
hpack_dynamic_table *dynamic_table;

//finds entry in dynamic table
//entry is a pair name-value
headed_pair *dynamic_find_entry(uint32_t index)
{
    uint32_t table_index = (dynamic_table->next + dynamic_table->table_length - (index - 61)) % dynamic_table->table_length;

    return dynamic_table->table[table_index];
}

//general method to find an entry in the table
//entry is a pair name-value
//return -1 in case of error, returns 0 and copy the entry values into name and value buffers
int find_entry(uint32_t index, char *name, char *value)
{
    const char *table_name; //add const before char to resolve compilation warnings
    const char *table_value;

    if (index >= FIRST_INDEX_DYNAMIC) {
        headed_pair *entry = dynamic_find_entry(index);
        table_name = entry->name;
        table_value = entry->value;
    }
    else {
        index--; //because static table begins at index 1
        table_name = static_header_name_table[index];
        table_value = static_header_value_table[index];
    }
    if (strncpy(name, table_name, strlen(table_name)) <= (char *)0) {
        ERROR("Error en strncpy");
        return -1;
    }
    if (strncpy(value, table_value, strlen(table_value)) <= (char *)0) {
        ERROR("Error en strncpy");
        return -1;
    }
    return 0;

}
