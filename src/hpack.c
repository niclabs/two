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


/*
 * Function: find_entry_name_and_value
 * finds an entry (pair name-value) in either the static or dynamic table_length
 * Input:
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 *      -> *value: buffer where the value of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);

/*
 * Function: find_entry_name
 * finds an entry name in either the static or dynamic table_length
 * Input:
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);

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
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);

    for (uint16_t i = 0; i < str_length; i++) {
        str[i] = (char)encoded_string[str_length_size + i];
    }
    return str_length + str_length_size;
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
        int8_t can_read_bits = hpack_utils_check_can_read_buffer(bit_position, i, encoded_string_size);
        if (can_read_bits < 0) {
            return -1;
        }
        uint32_t result =  hpack_utils_read_bits_from_bytes(bit_position, i, encoded_string);
        /*int8_t rc = read_bits_from_bytes(bit_position, i, encoded_string, encoded_string_size, &result);
           if (rc < 0) {
            ERROR("Error while trying to read bits from encoded_string in decode_huffman_word");
            return -1;
           }*/
        encoded_word.code = result;
        encoded_word.length = i;
        uint8_t decoded_sym = 0;

        int8_t rc = hpack_huffman_decode(&encoded_word, &decoded_sym);

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
    uint32_t str_length_size = hpack_utils_encoded_integer_size(str_length, 7);
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
                    return str_length + str_length_size;
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
    return str_length + str_length_size;
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

int decode_indexed_header_field(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));

    if (find_entry_name_and_value(dynamic_table, index, name, value) == -1) {
        ERROR("Error en find_entry");
        return -1;
    }
    pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(INDEXED_HEADER_FIELD));
    return pointer;
}

int decode_literal_header_field_with_incremental_indexing(hpack_dynamic_table_t *dynamic_table, uint8_t *header_block, char *name, char *value)
{
    int pointer = 0;
    uint32_t index = decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));//decode index

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
        if (find_entry_name(dynamic_table, index, name) == -1) {
            ERROR("Error en find_entry");
            return -1;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING));
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
    uint32_t index = decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index

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
        if (find_entry_name(dynamic_table, index, name) == -1) {
            ERROR("Error en find_entry ");
            return -1;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));
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
    uint32_t index = decode_integer(header_block, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index

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
        int rc = find_entry_name(dynamic_table, index, name);
        if (rc == -1) {
            ERROR("Error en find_entry ");
            return -1;
        }
        pointer += hpack_utils_encoded_integer_size(index, hpack_utils_find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));
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
            ERROR("Error in decode_indexed_header_field ");
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
    else if (preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
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

    char tmp_name[16];
    char tmp_value[32];

    while (pointer < header_block_size) {
        memset(tmp_name, 0, 16);
        memset(tmp_value, 0, 32);
        hpack_preamble_t preamble = hpack_utils_get_preamble(header_block[pointer]);
        int rc = decode_header(dynamic_table, header_block + pointer, preamble, tmp_name, tmp_value);
        headers_add(headers, tmp_name, tmp_value);

        if (rc < 0) {
            ERROR("Error in decode_header ");
            return -1;
        }

        pointer += rc;
    }
    if (pointer > header_block_size) {
        ERROR("Error decoding header block ... ");
        return -1;
    }
    return pointer;
}

/*
 * Function: hpack_encoder_encode
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
int encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index, char *name_string, uint8_t name_huffman_bool, char *value_string, uint8_t value_huffman_bool,  uint8_t *encoded_buffer)
{
    return hpack_encoder_encode(preamble, max_size, index, name_string,  name_huffman_bool, value_string, value_huffman_bool, encoded_buffer);
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
    memset(dynamic_table, 0, sizeof(hpack_dynamic_table_t));
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
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 * Output:
 *      0 if success, -1 in case of Error
 */
header_pair_t dynamic_find_entry(hpack_dynamic_table_t *dynamic_table, uint32_t index)
{
    uint32_t table_index = (dynamic_table->next + dynamic_table->table_length - (index - 61)) % dynamic_table->table_length;

    return dynamic_table->table[table_index];
}

/*
 * Function: find_entry_name_and_value
 * finds an entry (pair name-value) in either the static or dynamic table_length
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 *      -> *value: buffer where the value of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value)
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
        int8_t rc = hpack_tables_static_find_name_and_value(index, name, value);
        if (rc < 0) {
            ERROR("The index was greater than the size of the static table");
            return -1;
        }
        return 0;
    }
    strncpy(name, table_name, strlen(table_name));
    strncpy(value, table_value, strlen(table_value));
    return 0;

}

/*
 * Function: find_entry_name
 * finds an entry name in either the static or dynamic table_length
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    const char *table_name; //add const before char to resolve compilation warnings

    if (index >= FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }
        header_pair_t entry = dynamic_find_entry(dynamic_table, index);
        table_name = entry.name;
    }
    else {
        int8_t rc = hpack_tables_static_find_name(index, name);
        if (rc < 0) {
            ERROR("The index was greater than the size of the static table");
            return -1;
        }
        return 0;
    }
    strncpy(name, table_name, strlen(table_name));
    return 0;

}
