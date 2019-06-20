#include "hpack.h"
#include "logging.h"
#include <math.h>
#include "hpack_utils.h"
#include <stdio.h>
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




/*returns the amount of octets used to encode a int num with a prefix p*/
uint32_t get_octets_length(uint32_t num, uint8_t prefix){
    //Serial.println("getOctetsLength");
    uint8_t p = 255;
    p = p << (8 - prefix);
    p = p >> (8 - prefix);
    if(num >= p){
        uint32_t k = log(num - p) / log(128);
        return k + 2;
    }else{
        return 1;
    }
};

/*returns the size of the encoded integer with prefix*/
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
        int k = log(integer) / log(128);
        octets_size = k + 2;
    }
    return octets_size;
}




/*
 * encode an integer with the given prefix
 * and save the encoded integer in "encoded_integer"
 * encoded_integer must be an array of the size calculated by encoded_integer_size
 * returns the encoded_integer_size
 */
int encode_integer(uint32_t integer, uint8_t prefix, uint8_t* encoded_integer){
    int octets_size = 0;
    uint32_t max_first_octet = (1<<prefix)-1;
    if(integer < max_first_octet){
        encoded_integer[0] = (uint8_t)(integer << (8 - prefix));
        encoded_integer[0] = (uint8_t)encoded_integer[0] >> (8 - prefix);
        octets_size = 1;
    }else{
        uint8_t b0 = 255;
        b0 = b0 << (8 - prefix);
        b0 = b0 >> (8 - prefix);
        integer = integer - b0;
        int k = log(integer)/log(128);
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
            //TODO not implemented yet
            ERROR("Not implemented yet!");
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


/*int decode(uint8_t *encoded_buffer){

}*/


hpack_preamble_t get_preamble(uint8_t preamble){
    if(preamble&INDEXED_HEADER_FIELD){
        return INDEXED_HEADER_FIELD;
    }
    if(preamble&LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING){
        return LITERAL_HEADER_FIELD_WITH_INCREMENTAL_INDEXING;
    }
    if(preamble&DYNAMIC_TABLE_SIZE_UPDATE){
        return DYNAMIC_TABLE_SIZE_UPDATE;
    }
    if(preamble&LITERAL_HEADER_FIELD_NEVER_INDEXED){
        return LITERAL_HEADER_FIELD_NEVER_INDEXED;
    }
    if(preamble<16) {
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
uint32_t decode_integer(uint8_t* bytes, uint8_t prefix){
    int pointer = 0;
    uint8_t b0 = bytes[0];
    b0 = b0<<(8-prefix);
    b0 = b0>>(8-prefix);
    uint8_t p = 255;
    p = p<<(8-prefix);
    p = p>>(8-prefix);
    if(b0!=p){
        return (uint32_t) b0;
    }else{
        uint32_t integer = (uint32_t)p;
        uint32_t depth = 0;
        for(uint32_t i = 1; ; i++){
            uint8_t bi = bytes[pointer+i];
            if(!(bi&(uint8_t)128)){
                integer += (uint32_t)bi*((uint32_t)1<<depth);
                return integer;
            }else{
                bi = bi<<1;
                bi = bi>>1;
                integer += (uint32_t)bi*((uint32_t)1<<depth);
            }
            depth = depth+7;
        }
    }
    return -1;
}

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
}
int decode_literal_header_field_without_indexing(uint8_t* header_block, char* name, char* value){
    int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_WITHOUT_INDEXING));//decode index
    if(index == 0){
        pointer += 1;
        //decode huffman name
        //decode name length
        int name_length = decode_integer(header_block+pointer, 7);
        pointer += encoded_integer_size(name_length,7);
        //decode name
        char* rc = strncpy(name,(char*)header_block+pointer, name_length);
        if(rc<=(char*)0){
            ERROR("Error en strncpy");
            return -1;
        }
        pointer += name_length;
    }
    else{
        //TODO find name in table
        ERROR("NOt implemented yet");
        return -1;
    }
    //decode value length
    int value_length = decode_integer(header_block+pointer, 7);
    pointer += encoded_integer_size(value_length,7);
    //decode value
    strncpy(value,(char*)header_block+pointer, value_length);
    pointer += value_length;

    return pointer;
}


int decode_literal_header_field_never_indexed(uint8_t* header_block, char* name, char* value){
    //int pointer = 0;
    uint32_t index = decode_integer(header_block, find_prefix_size(LITERAL_HEADER_FIELD_NEVER_INDEXED));//decode index
    if(index == 0){
        //TODO
        (void)name;
        //decode huffman name
        //decode name length
        //decode name
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
}
int decode_dynamic_table_size(uint8_t* header_block){
    //int pointer = 0;
    uint32_t new_table_size = decode_integer(header_block, find_prefix_size(DYNAMIC_TABLE_SIZE_UPDATE));//decode index
    //TODO modify dynamic table size and dynamic table if necessary.
    (void)new_table_size;
    ERROR("Not implemented yet.");
    return -1;
}


int decode_header(uint8_t* bytes, hpack_preamble_t preamble, char* name, char* value){
    if(preamble == LITERAL_HEADER_FIELD_WITHOUT_INDEXING){
        int rc = decode_literal_header_field_without_indexing(bytes,name,value);
        if(rc<0){
            ERROR("Error in decode_literal_header_field_without_indexing");
        }
        return rc;
    }else{
        ERROR("Not implemented yet.");
        return -1;
    }
}

int decode_header_block(uint8_t* header_block, uint8_t header_block_size, headers_lists_t* h_list){
    int pointer = 0;

    int header_counter = 0;
    while(pointer < header_block_size) {
        hpack_preamble_t preamble = get_preamble(header_block[pointer]);
        int rc = decode_header(header_block + pointer, preamble, h_list->header_list_in[h_list->header_list_count_in + header_counter].name,
                               h_list->header_list_in[h_list->header_list_count_in + header_counter].value);

        if(rc<0){
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









