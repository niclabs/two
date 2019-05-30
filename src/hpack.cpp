#include "hpack.h"
#include "logging.h"
#include <math.h>
/*
 *
 * returns compressed headers size or -1 if error
 */
int compress_hauffman(char* headers, int headers_size, uint8_t* compressed_headers){
    (void)headers;
    (void)headers_size;
    (void)compressed_headers;
    ERROR("compress_hauffman: not implemented yet!");
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



/*
 * pass a byte b to string bits in str
 * "str" must be a char[9] for allocate string with 8chars + '/0'
 * Returns 0 if ok.
 */
int byte_to_8bits_string(uint8_t b, char* str){
    uint8_t aux = (uint8_t)128;
    for(uint8_t i = 0; i < 8; i++){
        if(b & aux){
            str[i] = '1';
        }else{
            str[i] = '0';
        }
        aux = aux >> 1;
    }
    str[8] = '\0';
    return 0;
};




/*returns the amount of octets used to encode a int num with a prefix p*/
uint32_t get_octets_length(uint32_t num, uint8_t prefix){
    //Serial.println("getOctetsLength");
    uint8_t p = 255;
    p = p << (8 - prefix);
    p = p >> (8 - prefix);
    if(num >= p){
        uint32_t k = 0;//TODO fix LOG log(num - p) / log(128);
        return k + 2;
    }else{
        return 1;
    }
};


int encoded_integer_size(uint32_t integer, uint8_t prefix){
    int octets_size;
    uint32_t max_first_octet = (1<<prefix)-1;
    if(integer < max_first_octet) {
        octets_size = 1;
    }else {//GEQ than 2^n-1
        uint8_t b0 = 255;
        b0 = b0 << (8 - prefix);
        b0 = b0 >> (8 - prefix);
        integer = integer - b0;
        int k = 0;//TODO fix LOG log(integer) / log(128);
        octets_size = k + 2;
    }
    return octets_size;
}


/*
 * encode an integer wit the given prefix
 * and save the encoded integer in "encoded_integer"
 * encoded_integer must be an array of the size calculated by encoded_integer_size
 * returns the encoded_intege_size
 */
int encode_integer(uint32_t integer, uint8_t prefix, uint8_t* encoded_integer){
    int octets_size;
    uint32_t max_first_octet = (1<<prefix)-1;
    if(integer < max_first_octet){
        encoded_integer[0] = (uint8_t)(integer << (8 - prefix));
        encoded_integer[0] = (uint8_t)encoded_integer[0] >> (8 - prefix);
    }else{
        uint8_t b0 = 255;
        b0 = b0 << (8 - prefix);
        b0 = b0 >> (8 - prefix);
        integer = integer - b0;
        int k = 0;//TODO fix LOG log(integer)/log(128);
        octets_size = k+2;
        encoded_integer[0] = b0;

        int i = 1;
        while(integer >= 128){
            uint32_t encoded = integer % 128;
            encoded += 128;
            encoded_integer[i] = (uint8_t) encoded;
            i++;
            integer = integer/128;
        }
        uint8_t bi = (uint8_t) integer & 0xff;
        encoded_integer[i] = bi;
    }
    return octets_size;
};



int encode_non_huffman_string(char* str, uint8_t* encoded_string){
    int str_length = strlen(str);
    int encoded_string_length_size = encode_integer(str_length,7,encoded_string); //encode integer(string size) with prefix 7. this puts the encoded string size in encoded string
    for(int i = 0; i< str_length; i++){//TODO check if strlen is ok to use here
        encoded_string[i+encoded_string_length_size]=str[i];
    }
    return str_length+encoded_string_length_size;
}

int encode_huffman_word(char* str, uint8_t* encoded_word){
    (void) str;
    (void) encoded_word;
    ERROR("Not implemented yet");
    return -1;
}



int encode_huffman_string(char* str, uint8_t* encoded_string){

    uint8_t encoded_word[64];
    int encoded_word_length = encode_huffman_word(str, encoded_word); //save de encoded word in "encoded_word" and returns the length of the encoded word

    if(encoded_word_length>=64){
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

int encode_string(char* str, uint8_t huffman, uint8_t* encoded_string){
    if(huffman){
        return encode_huffman_string(str, encoded_string);
    }else{
        return encode_non_huffman_string(str,encoded_string);
    }
};

uint8_t find_prefix_size(hpack_preamble_t octet){
    if((INDEXED_HEADER_FIELD&octet)==INDEXED_HEADER_FIELD){
        return (uint8_t)7;
    }
    if((LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING&octet)==LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING){
        return (uint8_t)6;
    }
    if((DYNAMIC_TABLE_SIZE_UPDATE&octet)==DYNAMIC_TABLE_SIZE_UPDATE){
        return (uint8_t)5;
    }
    return (uint8_t)4; /*LITERAL_HEADER_FIELD_WITHOUT_INDEXING and LITERAL_HEADER_FIELD_NEVER_INDEXED*/
};

int pack_huffman_string_and_size(char* string,uint8_t* encoded_buffer){
    uint8_t encoded_string[64];
    int pointer = 0;
    int encoded_size = encode_huffman_string(string, encoded_string);
    if(encoded_size>=64){
        ERROR("string encoded size too big");
        return -1;
    }
    int encoded_size_size = encode_integer(encoded_size,7,encoded_buffer+pointer);
    encoded_buffer[pointer] |=(uint8_t)128; /*set hauffman bit*/
    pointer += encoded_size_size;
    for(int i = 0; i< encoded_size;i++){
        encoded_buffer[pointer+i]=encoded_string[i];
    }
    pointer += encoded_size;
    return pointer;
}

int pack_non_huffman_string_and_size(char* string, uint8_t* encoded_buffer){
    int pointer = 0;
    int string_size = strlen(string);
    int encoded_size = encode_integer(string_size,7,encoded_buffer+pointer);
    pointer += encoded_size;
    for(int i = 0; i< string_size;i++){
        encoded_buffer[pointer+i] = string[i];
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
int encode_literal_header_field_new_name( char* name_string, uint8_t name_huffman_bool, char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;

    if(name_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(name_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(name_string,encoded_buffer+pointer);
    }
    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }
    return pointer;
}

int encode_literal_header_field_indexed_name( char* value_string, uint8_t value_huffman_bool,uint8_t* encoded_buffer){
    int pointer = 0;

    if(value_huffman_bool!=0){
        pointer += pack_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }else{
        pointer += pack_non_huffman_string_and_size(value_string,encoded_buffer+pointer);
    }
    return pointer;
}

int encode(hpack_preamble_t preamble, uint32_t max_size, uint32_t index,char* value_string, uint8_t value_huffman_bool, char* name_string, uint8_t name_huffman_bool, uint8_t* encoded_buffer){
    if(preamble == DYNAMIC_TABLE_SIZE_UPDATE){ // dynamicTableSizeUpdate
        int encoded_max_size_length = encode_integer(max_size, 5,encoded_buffer);
        encoded_buffer[0] |= preamble;
        return encoded_max_size_length;
    }else{
        uint8_t prefix = find_prefix_size(preamble);
        int pointer = 0;
        pointer += encode_integer(index, prefix, encoded_buffer + pointer);
        encoded_buffer[0] |= preamble;//set first bit
        if(preamble == (uint8_t)INDEXED_HEADER_FIELD){/*indexed header field representation in static or dynamic table*/

            return pointer;
        }else{
            if(index==(uint8_t)0){
                pointer += encode_literal_header_field_new_name( name_string, name_huffman_bool, value_string, value_huffman_bool, encoded_buffer + pointer);
            }else{
                pointer += encode_literal_header_field_indexed_name( value_string, value_huffman_bool, encoded_buffer + pointer);
            }
            return pointer;
        }
    }
};
