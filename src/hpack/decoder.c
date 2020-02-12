#include <stdint.h> /* for int8_t, int32_t*/
#include <string.h> /* for memset, NULL*/

#include "hpack/decoder.h"
#include "hpack/huffman.h"
#include "hpack/utils.h"

#define LOG_MODULE LOG_MODULE_HPACK
#include "logging.h"

/*
 * Function: hpack_decoder_decode_integer
 * Decodes an integer using prefix bits for first byte
 * Input:
 *      -> *bytes: Bytes storing encoded integer
 *      -> prefix: size of prefix used to encode integer
 * Output:
 *      returns the decoded integer if succesful, -1 otherwise
 */
int32_t hpack_decoder_decode_integer(const uint8_t *bytes, uint8_t prefix)
{
    uint8_t b0 = bytes[0];

    b0        = b0 << (8u - prefix);
    b0        = b0 >> (8u - prefix);
    uint8_t p = 255u;
    // p = p << (8u - prefix);
    p = p >> (8u - prefix);
    if (b0 != p) {
        /*Special case, when HPACK_MAXIMUM_INTEGER is less than 256
         * it will evaluate whether b0 is within limits*/
#if (HPACK_MAXIMUM_INTEGER < 256)
        if (b0 < HPACK_MAXIMUM_INTEGER) {
            return (int32_t)b0;
        } else {
            return -1;
        }
#else
        return (int32_t)b0;
#endif
    } else {
        uint32_t integer = (uint32_t)p;
        uint32_t depth   = 0;
        for (uint32_t i = 1;; i++) {
            uint8_t bi = bytes[i];
            if (!(bi & (uint8_t)128)) {
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
                if (integer > HPACK_MAXIMUM_INTEGER) {
                    DEBUG("Integer is %u:", (unsigned int)integer);
                    return -1;
                } else {
                    return integer;
                }
            } else {
                bi = bi << 1u;
                bi = bi >> 1u;
                integer += (uint32_t)bi * ((uint32_t)1 << depth);
            }
            depth = depth + 7;
        }
    }
}

/*
 * Function: hpack_decoder_read_bits_from_bytes
 * Reads bits from a buffer of bytes (max number of bits it can read is 32).
 * Before calling this function, the caller has to check if the number of bits
 * to read from the buffer don't exceed the size of the buffer, use
 * hpack_utils_can_read_buffer to check this condition Input:
 *      -> current_bit_pointer: The bit from where to start reading (inclusive)
 *      -> number_of_bits_to_read: The number of bits to read from the buffer
 *      -> *buffer: The buffer containing the bits to read
 *      -> buffer_size: Size of the buffer
 * output: returns the bits read from the buffer
 */
uint32_t hpack_decoder_read_bits_from_bytes(uint16_t current_bit_pointer,
                                            uint8_t number_of_bits_to_read,
                                            const uint8_t *buffer)
{
    uint16_t byte_offset = (uint16_t)(current_bit_pointer / 8u);
    uint16_t bit_offset  = (uint16_t)(current_bit_pointer - 8u * byte_offset);
    uint16_t num_bytes =
      (uint16_t)(((number_of_bits_to_read + current_bit_pointer - 1u) / 8) -
                 (current_bit_pointer / 8u) + 1u);

    uint8_t first_byte_mask = 255u;

    first_byte_mask >>= bit_offset;

    uint32_t code = (uint8_t)(first_byte_mask & buffer[byte_offset]);
    uint8_t last_bit_offset =
      (uint8_t)((8u * (num_bytes + byte_offset)) -
                (current_bit_pointer + number_of_bits_to_read));

    if (num_bytes == 1) {
        code >>= last_bit_offset;
        return code;
    } else {
        for (int i = 1; i < num_bytes - 1; i++) {
            code <<= 8u;
            code |= buffer[i + byte_offset];
        }

        uint8_t last_byte_mask = (uint8_t)(255u << last_bit_offset);
        uint8_t last_byte =
          last_byte_mask & buffer[byte_offset + num_bytes - 1];
        last_byte >>= last_bit_offset;
        code <<= (8u - last_bit_offset);
        code |= last_byte;
    }
    return code;
}

#if (INCLUDE_HUFFMAN_COMPRESSION)

/*
 * Function: hpack_decoder_check_huffman_padding
 * Checks if the last byte has correct padding
 * Input:
 *      bits_left: bits NOT read from the last byte of the buffer
 *      *encoded_buffer: Buffer containing encoded bytes.
 *      str_length: Length of the buffer
 * Output:
 *      returns 0 if padding of last byte is correct, if it fails throws an
 * error.
 */

hpack_error_t hpack_decoder_check_huffman_padding(uint8_t bits_left,
                                                  const uint8_t *encoded_buffer,
                                                  uint32_t str_length)
{
    // uint8_t bits_left = (uint8_t)(8 * str_length - bit_position);
    uint8_t last_byte = encoded_buffer[str_length - 1];

    if (bits_left < 8) {
        uint8_t mask =
          (uint8_t)((1u << bits_left) - 1u); /*padding of encoding*/
        if ((last_byte & mask) == mask) {
            return HPACK_NO_ERROR;
        } else {
            DEBUG("Last byte is %d", last_byte);
            ERROR("Decoding error: The compressed header padding contains a "
                  "value different from the EOS symbol");
            return HPACK_COMPRESSION_ERROR;
        }
    } else {
        /*Check if it has a padding greater than 7 bits*/
        uint8_t mask = 255u;
        if (last_byte == mask) {
            ERROR("Decoding error: The compressed header has a padding greater "
                  "than 7 bits");
            return HPACK_COMPRESSION_ERROR;
        }
        DEBUG("Couldn't decode string in hpack_decoder_decode_huffman_string");
        return HPACK_COMPRESSION_ERROR;
    }
}

/*
 * Function: hpack_decoder_decode_huffman_word
 * Given the bit position from where to start to read, tries to decode de bits
 * using different lengths (from smallest (5) to largest(30)) and stores the
 * result byte in *str. This function is meant to be used internally by
 * hpack_decoder_decode_huffman_string. Input:
 *      -> *str: Pointer to byte to store the result of the decompression
 * process
 *      -> *encoded_string: Buffer storing compressed representation of string
 * to decompress
 *      -> encoded_string_size: Size (in octets) given by header, this is not
 * the same as the decompressed string size
 *      -> bit_position: Starts to decompress from this bit in the
 * *encoded_string Output: Stores the result in the first byte of str and
 * returns the size in bits of the encoded word (e.g. 5 or 6 or 7 ...), returns
 * the number of bytes read from encoded_buffer is successful, if it fails
 * throws an error.
 */
int32_t hpack_decoder_decode_huffman_word(char *str, uint8_t *encoded_string,
                                          uint32_t encoded_string_size,
                                          uint16_t bit_position)
{
    huffman_encoded_word_t encoded_word;
    uint8_t length = 30;
    uint32_t result;
    uint8_t bits_left = (uint8_t)((8u * encoded_string_size) - bit_position);

    /*Check if can read buffer*/
    if (bit_position + length > 8 * encoded_string_size) {
        if (bits_left < 5) {
            return hpack_decoder_check_huffman_padding(
              bits_left, encoded_string, encoded_string_size);
        } else {
            uint8_t number_of_padding_bits = (uint8_t)(length - bits_left);
            uint32_t padding = (1u << (number_of_padding_bits)) - 1u;
            result = hpack_decoder_read_bits_from_bytes(bit_position, bits_left,
                                                        encoded_string);
            result <<= number_of_padding_bits;
            result |= padding;
        }
    } else {
        result = hpack_decoder_read_bits_from_bytes(bit_position, length,
                                                    encoded_string);
    }

    encoded_word.code = result;
    // encoded_word.length = 30;
    uint8_t decoded_sym = 0;

    int8_t bit_size = hpack_huffman_decode(&encoded_word, &decoded_sym);

    if (bit_size >= 0) { /*Code is found on huffman tree*/
        str[0] = (char)decoded_sym;
        return bit_size;
    }
    INFO("Couldn't read bits in hpack_decoder_decode_huffman_word");
    return hpack_decoder_check_huffman_padding(bits_left, encoded_string,
                                               encoded_string_size);
}
#endif

#if (INCLUDE_HUFFMAN_COMPRESSION)

/*
 * Function: hpack_decoder_decode_huffman_string
 * Tries to decode encoded_string (compressed using huffman) and store the
 * result in *str Input:
 *      -> *str: Buffer to store the result of the decompression process, this
 * buffer must be bigger than encoded_string
 *      -> *encoded_string: Buffer containing a string compressed using Huffman
 * Compression
 *      -> str_length: length of the string to decode
 * Output:
 *      Stores the decompressed version of encoded_string in str if successful
 * and returns the number of bytes read of encoded_string, if it fails to decode
 * the string the return value is -1
 */
int32_t hpack_decoder_decode_huffman_string(char *str, uint32_t str_length,
                                            uint8_t *encoded_string,
                                            uint32_t encoded_str_length)
{
    uint32_t str_length_size =
      hpack_utils_encoded_integer_size(encoded_str_length, 7);
    uint16_t bit_position = 0;

    for (uint16_t i = 0; ((bit_position - 1) / 8) < (int32_t)encoded_str_length;
         i++) {
        if (str_length < i) {
            return HPACK_MEMORY_ERROR;
        }
        int32_t word_length = hpack_decoder_decode_huffman_word(
          str + i, encoded_string, encoded_str_length, bit_position);
        if (word_length == 0) {
            break;
        }
        if (word_length < 0) { // ERROR
            return word_length;
        }
        bit_position += word_length;
    }
    return encoded_str_length + str_length_size;
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
 *      return the number of bytes written in str if successful or -1 otherwise
 */
int32_t hpack_decoder_decode_non_huffman_string(char *str, uint32_t str_length,
                                                uint8_t *encoded_string,
                                                uint32_t encoded_str_length)
{
    uint32_t str_length_size =
      hpack_utils_encoded_integer_size(encoded_str_length, 7);

    if (encoded_str_length > str_length) {
        return HPACK_MEMORY_ERROR;
    }
    for (uint32_t i = 0; i < encoded_str_length; i++) {
        str[i] = (char)encoded_string[i];
    }
    return encoded_str_length + str_length_size;
}

/*
 * Function: hpack_decoder_decode_string
 * Decodes an string according to its huffman bit, if it's 1 the string is
 * decoded using huffman decompression if it's 0 it copies the content of the
 * encoded_buffer to str. Input:
 *      -> *str: Buffer to store the result of the decoding process
 *      -> *encoded_buffer: Buffer containing the string to decode
 * Output:
 *      Returns the number of bytes read from encoded_buffer in the decoding
 * process if successful, if the process fails the function returns -1
 */
int32_t hpack_decoder_decode_string(char *str, uint32_t str_length,
                                    uint8_t *encoded_buffer,
                                    uint32_t encoded_buffer_length,
                                    uint8_t huffman_bit)
{
    if (huffman_bit) {
#if (INCLUDE_HUFFMAN_COMPRESSION)
        return hpack_decoder_decode_huffman_string(
          str, str_length, encoded_buffer, encoded_buffer_length);
#else
        ERROR("Not implemented: Cannot decode a huffman compressed header");
        return INTERNAL_ERROR;
#endif
    } else {
        return hpack_decoder_decode_non_huffman_string(
          str, str_length, encoded_buffer, encoded_buffer_length);
    }
}

/*
 * Function: hpack_decoder_decode_indexed_header_field
 * Decodes an indexed header field, it searches for name and value of an entry
 * in hpack tables Input:
 *      -> *states: struct in which is stored the header to be decoded and the
 * temporal buffers to save the entry Output:
 *      -> returns the amount of octets of the header, less than 0 in case of
 * error
 */
int hpack_decoder_decode_indexed_header_field(
  hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header,
  char *tmp_name, char *tmp_value)
{
    int pointer = 0;
    int8_t rc   = hpack_tables_find_entry_name_and_value(
      dynamic_table, encoded_header->index, tmp_name, tmp_value);

    if (rc < 0) {
        DEBUG("Error en find_entry %d", rc);
        return rc;
    }

    pointer += hpack_utils_encoded_integer_size(
      encoded_header->index,
      hpack_utils_find_prefix_size(encoded_header->preamble));
    return pointer;
}

/*
 * Function: hpack_decoder_decode_literal_header_field
 * Decodes a literal header field, the entry info comes directly in the header
 * block Input:
 *      -> *states: struct in which is stored the header to be decoded and the
 * temporal buffers to save the entry Output:
 *      -> returns the amount of octets of the header, less than 0 in case of
 * error
 */
int hpack_decoder_decode_literal_header_field(
  hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header,
  char *tmp_name, char *tmp_value)
{
    int pointer = 0;

    // New name
    if (encoded_header->index == 0) {
        pointer += 1;
        DEBUG("Decoding a new name compressed header");
        int32_t rc = hpack_decoder_decode_string(
          tmp_name, HPACK_HEADER_NAME_LEN, encoded_header->name_string,
          encoded_header->name_length, encoded_header->huffman_bit_of_name);
        if (rc < 0) {
            DEBUG("Error while trying to decode string in "
                  "hpack_decoder_decode_literal_header_field_never_indexed");
            return rc;
        }
        pointer += rc;
    } else {
        // find entry in either static or dynamic table_length
        DEBUG("Decoding an indexed name compressed header");
        int8_t rc = hpack_tables_find_entry_name(
          dynamic_table, encoded_header->index, tmp_name);
        DEBUG("RC IS %d", rc);
        if (rc < 0) {
            DEBUG("Error en find_entry ");
            return rc;
        }
        pointer += hpack_utils_encoded_integer_size(
          encoded_header->index,
          hpack_utils_find_prefix_size(encoded_header->preamble));
    }

    int32_t rc = hpack_decoder_decode_string(
      tmp_value, HPACK_HEADER_VALUE_LEN, encoded_header->value_string,
      encoded_header->value_length, encoded_header->huffman_bit_of_value);
    if (rc < 0) {
        DEBUG("Error while trying to decode string in "
              "hpack_decoder_decode_literal_header_field_never_indexed");
        return rc;
    }
    pointer += rc;
    if (encoded_header->preamble ==
        LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
#if HPACK_INCLUDE_DYNAMIC_TABLE
        // Here we add it to the dynamic table
        rc = hpack_tables_dynamic_table_add_entry(dynamic_table, tmp_name,
                                                  tmp_value);
        if (rc < 0) {
            DEBUG("Couldn't add to dynamic table");
            return rc;
        }
#else
        DEBUG("Dynamic Table is not included, couldn't add header to table");
#endif
    }
    return pointer;
}

/*
 * Function: hpack_decode_dynamic_table_size_update
 * Decodes a header that represents a dynamic table size update
 * Input:
 *      -> *states: hpack main struct wrapper
 * Output:
 *      Returns the number of bytes decoded if successful or a value < 0 if an
 * error occurs.
 */
int hpack_decoder_decode_dynamic_table_size_update(
  hpack_dynamic_table_t *dynamic_table, hpack_encoded_header_t *encoded_header)
{
    DEBUG("New table size is %d", encoded_header->dynamic_table_size);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    int8_t rc = hpack_tables_dynamic_table_resize(
      dynamic_table, encoded_header->dynamic_table_size);
    if (rc < 0) {
        DEBUG("Dynamic table failed to resize");
        return rc;
    }
    return hpack_utils_encoded_integer_size(
      encoded_header->dynamic_table_size,
      hpack_utils_find_prefix_size(encoded_header->preamble));
#else
    (void)dynamic_table;
    (void)encoded_header;
    ERROR("Couldn't resize non-existent dynamic table, returning an "
          "INTERNAL_ERROR");
    return HPACK_MEMORY_ERROR;
#endif
}

/*
 * Function: get_huffman_bit
 * Gets the flag to check if word is huffman encoded or not
 * Input:
 *      -> num: Octet which represent the beginning of the number
 * Output:
 *      -> returns 0 in case of no huffman encoded, grower than 0 in case of
 * huffman encoded
 */
uint8_t get_huffman_bit(uint8_t num)
{
    return (uint8_t)(128u & num);
}

/*
 * Function: hpack_decoder_parse_encoded_header
 * Parses the header info from the header block to the encoded header struct
 * Input:
 *      -> *encoded_header: struct in which will be stored the header info
 *      -> *header_block: header block to be read
 *      -> header_size: size of the header block
 * Output:
 *      -> returns the amount of octets of the header, less than 0 in case of
 * error
 */
int32_t hpack_decoder_parse_encoded_header(
  hpack_encoded_header_t *encoded_header, uint8_t *header_block,
  int32_t header_size)
{
    int32_t pointer = 0;

    encoded_header->preamble = hpack_utils_get_preamble(header_block[pointer]);
    int32_t index            = hpack_decoder_decode_integer(
      &header_block[pointer],
      hpack_utils_find_prefix_size(encoded_header->preamble)); // decode index
    /*Integer exceed implementations limits*/
    if (index < 0) {
        ERROR("Integer exceeds implementations limits");
        return HPACK_COMPRESSION_ERROR;
    }
    encoded_header->index = (uint32_t)index;
    int32_t index_size    = hpack_utils_encoded_integer_size(
      encoded_header->index,
      hpack_utils_find_prefix_size(encoded_header->preamble));
    pointer += index_size;

    if (encoded_header->preamble == INDEXED_HEADER_FIELD) {
        return pointer;
    }

    if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        encoded_header->dynamic_table_size = (uint16_t)encoded_header->index;
        encoded_header->index              = 0;
        return pointer;
    }

    /*Bytes read exceeded the header block size specified  at the beginning*/
    if (pointer > header_size) {
        ERROR("Bytes read exceeded the header block size specified  at the "
              "beginning");
        return HPACK_COMPRESSION_ERROR;
    }

    if (encoded_header->index == 0) {
        encoded_header->huffman_bit_of_name =
          get_huffman_bit(header_block[pointer]);
        int32_t name_length =
          hpack_decoder_decode_integer(&header_block[pointer], 7);

        /*Integer exceed implementations limits*/
        if (name_length < 0) {
            ERROR("Integer exceeds implementations limits");
            return HPACK_COMPRESSION_ERROR;
        }
        encoded_header->name_length = (uint32_t)name_length;
        pointer +=
          hpack_utils_encoded_integer_size(encoded_header->name_length, 7);
        /*Bytes read exceeded the header block size specified  at the
         * beginning*/
        if (pointer > header_size) {
            ERROR("Bytes read exceeded the header block size specified  at the "
                  "beginning");
            return HPACK_COMPRESSION_ERROR;
        }

        encoded_header->name_string = &header_block[pointer];
        pointer += encoded_header->name_length;
        /*Bytes read exceeded the header block size specified  at the
         * beginning*/
        if (pointer > header_size) {
            ERROR("Bytes read exceeded the header block size specified  at the "
                  "beginning");
            return HPACK_COMPRESSION_ERROR;
        }
    }

    encoded_header->huffman_bit_of_value =
      get_huffman_bit(header_block[pointer]);
    int32_t value_length =
      hpack_decoder_decode_integer(&header_block[pointer], 7);
    /*Integer exceed implementations limits*/
    if (value_length < 0) {
        ERROR("Integer exceeds implementations limits");
        return HPACK_COMPRESSION_ERROR;
    }

    encoded_header->value_length = (uint32_t)value_length;
    pointer +=
      hpack_utils_encoded_integer_size(encoded_header->value_length, 7);
    /*Bytes read exceeded the header block size specified  at the beginning*/
    if (pointer > header_size) {
        ERROR("Bytes read exceeded the header block size specified  at the "
              "beginning");
        return HPACK_COMPRESSION_ERROR;
    }
    encoded_header->value_string = &header_block[pointer];
    pointer += encoded_header->value_length;
    /*Bytes read exceeded the header block size specified  at the beginning*/
    if (pointer > header_size) {
        ERROR("Bytes read exceeded the header block size specified  at the "
              "beginning");
        return HPACK_COMPRESSION_ERROR;
    }
    return pointer;
}

/* Function: hpack_decoder_decode_header
 * decodes a header according to the preamble
 * Input:
 *      -> *dynamic_table: table that could be modified by encoder or decoder,
 * it allocates headers
 *      -> *bytes: Buffer containing data to decode
 *      -> *name: Memory to store decoded name
 *      -> *value: Memory to store decoded value
 * Output:
 *      returns the amount of octets in which the pointer has moved to read all
 * the headers
 *
 */
int hpack_decoder_decode_header(hpack_dynamic_table_t *dynamic_table,
                                hpack_encoded_header_t *encoded_header,
                                char *tmp_name, char *tmp_value)
{
    if (encoded_header->preamble == INDEXED_HEADER_FIELD) {
        DEBUG("Decoding an indexed header field");
        return hpack_decoder_decode_indexed_header_field(
          dynamic_table, encoded_header, tmp_name, tmp_value);
    } else if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        DEBUG("Decoding a dynamic table size update");
        return hpack_decoder_decode_dynamic_table_size_update(dynamic_table,
                                                              encoded_header);
    } else if (encoded_header->preamble ==
                 LITERAL_HEADER_FIELD_WITHOUT_INDEXING ||
               encoded_header->preamble == LITERAL_HEADER_FIELD_NEVER_INDEXED ||
               encoded_header->preamble ==
                 LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING) {
        return hpack_decoder_decode_literal_header_field(
          dynamic_table, encoded_header, tmp_name, tmp_value);
    }

    else {
        ERROR("Error unknown preamble value: %d", encoded_header->preamble);
        return HPACK_INTERNAL_ERROR;
    }
}

/*
 * Function: hpack_check_eos_symbol
 * Checks if compressed header contains EOS symbol, which is a decoding error
 * Input:
 *      -> *encoded_buffer: Buffer to check
 *      -> buffer_length: The size of the buffer
 * Output:
 *      returns 0 in successful case, -1 in case of protocol error
 */
int8_t hpack_check_eos_symbol(uint8_t *encoded_buffer, uint8_t buffer_length)
{
    const uint32_t eos     = 0x3fffffff;
    uint8_t eos_bit_length = 30;

    for (uint16_t bit_position = 0;
         (bit_position + eos_bit_length - 1) < buffer_length * 8;
         bit_position++) { // search through all lengths possible

        uint32_t result = hpack_decoder_read_bits_from_bytes(
          bit_position, eos_bit_length, encoded_buffer);
        if ((result & eos) == eos) {
            ERROR(
              "Decoding Error: The compressed header contains the EOS Symbol");
            return HPACK_COMPRESSION_ERROR;
        }
    }

    return 0;
}

/*
 * Function: hpack_decoder_check_errors
 * Check protocol errors before starting decoding the header
 * Input:
 *      -> *encoded_header: Pointer to encoded_header to check
 * Output:
 *      returns 0 in successful case, -1 in case of protocol error
 */
int8_t hpack_decoder_check_errors(hpack_encoded_header_t *encoded_header)
{
    /*Row doesn't exist*/
    if (encoded_header->preamble == INDEXED_HEADER_FIELD &&
        encoded_header->index == 0) {
        ERROR("Decoding Error: Cannot retrieve a 0 index from hpack tables");
        return HPACK_COMPRESSION_ERROR;
    }

    /*Integer exceed implementations limits*/
    if (encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE &&
        encoded_header->dynamic_table_size > HPACK_MAXIMUM_INTEGER) {
        ERROR("Integer exceeds implementations limits");
        return HPACK_COMPRESSION_ERROR;
    }

    /*It's ok*/
    if (encoded_header->preamble == INDEXED_HEADER_FIELD ||
        encoded_header->preamble == DYNAMIC_TABLE_SIZE_UPDATE) {
        return 0;
    }

    if ((encoded_header->index == 0) && (encoded_header->name_length >= 4)) {
        /*Check if it's the EOS Symbol in name_string*/
        int8_t rc = hpack_check_eos_symbol(
          encoded_header->name_string, (uint8_t)encoded_header->name_length);
        if (rc < 0) {
            return rc;
        }
    }

    if (encoded_header->value_length >= 4) {
        /*Check if it's the EOS Symbol in value_string*/
        int8_t rc = hpack_check_eos_symbol(
          encoded_header->value_string, (uint8_t)encoded_header->value_length);
        if (rc < 0) {
            return rc;
        }
    }
    return 0;
}

/*
 * Function: hpack_decoder_decode_header_block
 * decodes an array of headers,
 * as it decodes one, the pointer of the headers moves forward
 * also has updates the decoded header lists, this is a wrapper function
 * Input:
 *      -> *dynamic_table: Pointer to dynamic table to store headers
 *      -> *header_block: Pointer to a sequence of octets (bytes)
 *      -> header_block_size: Size in bytes of the header block that will be
 * decoded
 *      -> headers: struct that allocates a list of headers (pair name and
 * value) Output: returns the amount of octets in which the pointer has move to
 * read all the headers
 */
int hpack_decoder_decode(hpack_dynamic_table_t *dynamic_table,
                         uint8_t *header_block, int32_t header_block_size,
                         header_list_t *headers)
{
    char tmp_name[HPACK_HEADER_NAME_LEN];
    char tmp_value[HPACK_HEADER_VALUE_LEN];

    int pointer = 0;

    uint8_t can_receive_dynamic_table_size_update = 1; // TRUE

    while (pointer < header_block_size) {
        hpack_encoded_header_t encoded_header = { 0 };
        memset(tmp_name, 0, HPACK_HEADER_NAME_LEN);
        memset(tmp_value, 0, HPACK_HEADER_VALUE_LEN);

        int bytes_read = hpack_decoder_parse_encoded_header(
          &encoded_header, header_block + pointer,
          (int32_t)(header_block_size - pointer));
        DEBUG("Decoding a %d", encoded_header.preamble);

        if (bytes_read < 0) {
            /*Error*/
            ERROR("Bytes read are negative after parsing encoded header");
            return bytes_read;
        }

        if (encoded_header.preamble != DYNAMIC_TABLE_SIZE_UPDATE) {
            can_receive_dynamic_table_size_update = 0; /*False*/
        } else { /*it's a dynamic table size update*/
            if (!can_receive_dynamic_table_size_update) {
                /*Error*/
                ERROR("Received a dynamic table size update after receiving 1 "
                      "or more headers");
                return HPACK_COMPRESSION_ERROR;
            }
        }
        pointer += bytes_read;

        int err = hpack_decoder_check_errors(&encoded_header);
        if (err < 0) {
            return err;
        }
        int rc = hpack_decoder_decode_header(dynamic_table, &encoded_header,
                                             tmp_name, tmp_value);
        if (rc < 0) {
            return rc;
        }
        if (tmp_name[0] != 0 && tmp_value[0] != 0) {
            rc = header_list_add(headers, tmp_name, tmp_value);
            if (rc < 0) {
                return rc;
            }
        }
    }
    if (pointer > header_block_size) {
        DEBUG("Error decoding header block, read %d bytes and "
              "header_block_size is %d",
              pointer, (int)header_block_size);
        return HPACK_INTERNAL_ERROR;
    }
    return pointer;
}
