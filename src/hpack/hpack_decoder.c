
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
int32_t hpack_decoder_decode_integer(uint8_t *bytes, uint8_t prefix)
{
    int pointer = 0;
    uint8_t b0 = bytes[0];

    b0 = b0 << (8 - prefix);
    b0 = b0 >> (8 - prefix);
    uint8_t p = 255;
    p = p << (8 - prefix);
    p = p >> (8 - prefix);
    if (b0 != p) {
        return (int32_t)b0;
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
            return INTERNAL_ERROR;
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
    return INTERNAL_ERROR;
}
#endif


/*
 * Function: hpack_decoder_check_huffman_padding
 * TODO:
 */

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
            return PROTOCOL_ERROR;
        }
    }
    else {
        /*Check if it has a padding greater than 7 bits*/
        uint8_t mask = 255u;
        if (last_byte == mask) {
            ERROR("Decoding error: The compressed header has a padding greater than 7 bits");
            return PROTOCOL_ERROR;
        }
        DEBUG("Couldn't decode string in hpack_decoder_decode_huffman_string");
        return INTERNAL_ERROR;
    }
}

#ifdef INCLUDE_HUFFMAN_COMPRESSION

/*
 * Function: hpack_decoder_decode_huffman_string_v2
 * Tries to decode encoded_string (compressed using huffman) and store the result in *str
 * Input:
 *      -> *str: Buffer to store the result of the decompression process, this buffer must be bigger than encoded_string
 *      -> *encoded_string: Buffer containing a string compressed using Huffman Compression
 *      -> str_length: length of the string to decode
 * Output:
 *      Stores the decompressed version of encoded_string in str if successful and returns the number of bytes read
 *      of encoded_string, if it fails to decode the string the return value is -1
 */
int32_t hpack_decoder_decode_huffman_string_v2(char *str, uint8_t *encoded_string, uint32_t str_length)
{
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
    uint16_t bit_position = 0;

    for (uint16_t i = 0; ((bit_position - 1) / 8) < (int32_t)str_length; i++) {
        int32_t word_length = hpack_decoder_decode_huffman_word(str + i, encoded_string, str_length, bit_position);
        if (word_length < 0) {
            return hpack_decoder_check_huffman_padding(bit_position, encoded_string, str_length, str_length_size);
        }
        bit_position += word_length;
    }
    return str_length + str_length_size;
}
#endif

/*
 * Function: hpack_decoder_decode_non_huffman_string
 * Decodes an Array of char not compressed with Huffman Compression
 * Input:
 *      -> *str: Buffer to store encoded array
 *      -> *encoded_string: Buffer containing encoded bytes
 *      -> str_length: length of the string to decode
 * Output:
 *      return the number of bytes written in str if succesful or -1 otherwise
 */
int32_t hpack_decoder_decode_non_huffman_string_v2(char *str, uint8_t *encoded_string, uint32_t str_length)
{
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);

    for (uint16_t i = 0; i < str_length; i++) {
        str[i] = (char)encoded_string[i];
    }
    return str_length + str_length_size;
}



/*
 * Function: hpack_decoder_decode_name_string
 * Decodes a name string according to its huffman bit, if it's 1 the string is decoded using huffman decompression
 * if it's 0 it copies the content of the encoded_buffer to str.
 * Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_buffer: Encoded string to decode
 *      -> length: Length of string to decode
 *      -> huffman_bit: Boolean, true if compressed with huffman tree
 * Output:
 *      Returns the number of bytes read from encoded_header in the decoding process if successful,
 *      if the process fails the function returns -2 or -1.
 */

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
int32_t hpack_decoder_decode_string_v2(char *str, uint8_t *encoded_buffer, uint32_t length, uint8_t huffman_bit)
{
    if (huffman_bit) {
#ifdef INCLUDE_HUFFMAN_COMPRESSION
        return hpack_decoder_decode_huffman_string_v2(str, encoded_buffer, length);
#else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return INTERNAL_ERROR;
#endif
    }
    else {
        return hpack_decoder_decode_non_huffman_string_v2(str, encoded_buffer, length);
    }
}


int hpack_decoder_decode_indexed_header_field_v2(hpack_states_t *states)
{
    int pointer = 0;
    int8_t rc = hpack_tables_find_entry_name_and_value(&states->dynamic_table,
                                                       states->encoded_header.index,
                                                       states->tmp_name, states->tmp_value);

    if (rc < 0) {
        DEBUG("Error en find_entry %d", rc);
        return rc;
    }

    pointer += hpack_utils_encoded_integer_size(states->encoded_header.index, hpack_utils_find_prefix_size(states->encoded_header.preamble));
    return pointer;
}



int hpack_decoder_decode_literal_header_field_v2(hpack_states_t *states)
{
    int pointer = 0;

    //New name
    if (states->encoded_header.index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc =
            hpack_decoder_decode_string_v2(states->tmp_name,
                                           states->encoded_header.name_string,
                                           states->encoded_header.name_length,
                                           states->encoded_header.huffman_bit_of_name);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
            return rc;
        }
        pointer += rc;
    }
    else {
        //find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(&states->dynamic_table,
                                                 states->encoded_header.index,
                                                 states->tmp_name);
        if (rc < 0) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(states->encoded_header.index, hpack_utils_find_prefix_size(states->encoded_header.preamble));
    }

    int32_t rc = hpack_decoder_decode_string_v2(states->tmp_value, states->encoded_header.value_string, states->encoded_header.value_length, states->encoded_header.huffman_bit_of_value);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in hpack_decoder_decode_literal_header_field_never_indexed");
        return rc;
    }
    pointer += rc;
    if (states->encoded_header.preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
        //Here we add it to the dynamic table
        int rc = hpack_tables_dynamic_table_add_entry(&states->dynamic_table, states->tmp_name, states->tmp_value);
        if (rc < 0) {
            DEBUG("Couldn't add to dynamic table");
            return rc;
        }
#else
        ERROR("Dynamic Table is not included, couldn't add header to table");
        return PROTOCOL_ERROR;
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

/*
 * Function: hpack_decode_dynamic_table_size_update_v2
 * Decodes a header that represents a dynamic table size update
 * Input:
 *      -> *states: hpack main struct wrapper
 * Output:
 *      Returns the number of bytes decoded if successful or a value < 0 if an error occurs.
 */
int hpack_decoder_decode_dynamic_table_size_update_v2(hpack_states_t *states)
{
    DEBUG("New table size is %d", states->encoded_header.dynamic_table_size);
    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    int8_t rc = hpack_tables_dynamic_table_resize(&states->dynamic_table, states->settings_max_table_size, states->encoded_header.dynamic_table_size);
    if (rc < 0) {
        DEBUG("Dynamic table failed to resize");
        return rc;
    }
    return hpack_utils_encoded_integer_size(states->encoded_header.dynamic_table_size, hpack_utils_find_prefix_size(states->encoded_header.preamble));
    #else
    return INTERNAL_ERROR;
    #endif
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

uint8_t get_huffman_bit(uint8_t num)
{
    return 128u & num;
}


int8_t hpack_decoder_parse_encoded_header(hpack_encoded_header_t *encoded_header, uint8_t *header_block, uint8_t header_size)
{
    int32_t pointer = 0;

    encoded_header->preamble = hpack_utils_get_preamble(header_block[pointer]);
    int16_t index = hpack_decoder_decode_integer(&header_block[pointer], hpack_utils_find_prefix_size(encoded_header->preamble));//decode index
    /*Integer exceed implementations limits*/
    if (index < 0) {
        ERROR("Integer exceeds implementations limits");
        return -1;
    }
    encoded_header->index = (uint32_t)index;
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

    /*Size is not sufficient for LITERAL HEADER FIELD*/
    if (pointer > header_size) {
        return -1;
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
        /*Size is not sufficient for LITERAL HEADER FIELD*/
        if (pointer > header_size) {
            return -1;
        }

        encoded_header->name_string = &header_block[pointer];
        pointer += encoded_header->name_length;
        /*Size is not sufficient for LITERAL HEADER FIELD*/
        if (pointer > header_size) {
            return -1;
        }
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
    /*Size is not sufficient for LITERAL HEADER FIELD*/
    if (pointer > header_size) {
        return -1;
    }
    encoded_header->value_string = &header_block[pointer];
    pointer += encoded_header->value_length;
    /*Size is not sufficient for LITERAL HEADER FIELD*/
    if (pointer > header_size) {
        return -1;
    }
    return pointer;
}

int hpack_decoder_decode_header_v2(hpack_states_t *states)
{
    if (states->encoded_header.preamble == INDEXED_HEADER_FIELD) {
        DEBUG("Decoding an indexed header field");
        return hpack_decoder_decode_indexed_header_field_v2(states);
    }
    else if (states->encoded_header.preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        DEBUG("Decoding a dynamic table size update");
        return hpack_decoder_decode_dynamic_table_size_update_v2(states);
    }
    else if (states->encoded_header.preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING
             || states->encoded_header.preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED
             || states->encoded_header.preamble == LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return hpack_decoder_decode_literal_header_field_v2(states);
    }

    else {
        ERROR("Error unknown preamble value: %d", states->encoded_header.preamble);
        return INTERNAL_ERROR;
    }
}

int hpack_check_eos_symbol(uint8_t *encoded_buffer, uint8_t buffer_length)
{
    const uint32_t eos = 0x3fffffff;
    uint8_t eos_bit_length = 30;

    for (int32_t bit_position = 0; (bit_position + eos_bit_length) / 8 < buffer_length; bit_position++) {     //search through all lengths possible

        uint32_t result = hpack_utils_read_bits_from_bytes(bit_position, eos_bit_length, encoded_buffer);
        if (result == eos) {
            ERROR("Decoding Error: The compressed header contains the EOS Symbol");
            return PROTOCOL_ERROR;
        }
    }

    return 0;
}

int hpack_decoder_check_errors(hpack_encoded_header_t *encoded_header)
{
    /*Row doesn't exist*/
    if (encoded_header->preamble == INDEXED_HEADER_FIELD && encoded_header->index == 0) {
        ERROR("Decoding Error: Cannot retrieve a 0 index from hpack tables");
        return PROTOCOL_ERROR;
    }

    /*Integer exceed implementations limits*/
    if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE && encoded_header->dynamic_table_size > HPACK_MAXIMUM_INTEGER_SIZE) {
        ERROR("Integer exceeds implementations limits");
        return PROTOCOL_ERROR;
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

void init_hpack_encoded_header_t(hpack_encoded_header_t *encoded_header)
{
    encoded_header->index = 0;
    encoded_header->name_length = 0;
    encoded_header->value_length = 0;
    encoded_header->name_string = 0;
    encoded_header->value_string = 0;
    encoded_header->huffman_bit_of_name = 0;
    encoded_header->huffman_bit_of_value = 0;
    encoded_header->dynamic_table_size = 0;
    encoded_header->preamble = 0;
}

int hpack_decoder_decode_header_block_v2(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)
{
    int pointer = 0;

    uint8_t can_receive_dynamic_table_size_update = 1;    //TRUE

    /*
       for (int i = 0; i < header_block_size; i++) {
        DEBUG("The byte is %x", header_block[i]);
       }*/
    while (pointer < header_block_size) {
        init_hpack_encoded_header_t(&states->encoded_header);
        memset(states->tmp_name, 0, MAX_HEADER_NAME_LEN);
        memset(states->tmp_value, 0, MAX_HEADER_VALUE_LEN);
        int bytes_read = hpack_decoder_parse_encoded_header(&states->encoded_header, header_block + pointer, header_block_size - pointer);
        DEBUG("Decoding a %d", states->encoded_header.preamble);

        if (bytes_read < 0) {
            /*Error*/
            return bytes_read;
        }

        if (states->encoded_header.preamble != DYNAMIC_TABLE_SIZE_UPDATE) {
            can_receive_dynamic_table_size_update = 0;      /*False*/
        }
        else {                                              /*it's a dynamic table size update*/
            if (!can_receive_dynamic_table_size_update) {
                /*Error*/
                return PROTOCOL_ERROR;
            }
        }
        pointer += bytes_read;

        int err = hpack_decoder_check_errors(&states->encoded_header);
        if (err < 0) {
            return err;
        }
        int rc = hpack_decoder_decode_header_v2(states);
        if (rc < 0) {
            return rc;
        }
        if (states->tmp_name[0] != 0 && states->tmp_value[0] != 0) {
            headers_add(headers, states->tmp_name, states->tmp_value);
        }
    }
    if (pointer > header_block_size) {
        DEBUG("Error decoding header block, read %d bytes and header_block_size is %d", pointer, header_block_size);
        return INTERNAL_ERROR;
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
int hpack_decoder_decode_header_block(hpack_states_t *states, uint8_t *header_block, uint8_t header_block_size, headers_t *headers)    //header_t* h_list, uint8_t * header_counter)
{
    return hpack_decoder_decode_header_block_v2(states, header_block, header_block_size, headers);

}
