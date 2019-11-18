#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "headers.h"
#include "logging.h"


/*
 * Function: headers_init
 * Initializes a headers struct
 * Input:
 *      -> *headers: struct which will be initialized
 * Output:
 *      (void)
 */
void headers_init(headers_t *headers)
{
    memset(headers->buffer, 0, sizeof(headers->buffer));

    headers->n_entries = 0;
    headers->size = 0;
}

/*
 * Function: headers_clean
 * Cleans headers list struct
 * Input:
 *      -> *headers: header list
 * Output:
 *      (void)
 */
void headers_clean(headers_t *headers)
{
    headers_init(headers);
}

/*
 * Function: headers_get
 * Get header value of the header list
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      String with the value of the header, NULL if it doesn't exist
 */
char *headers_get(headers_t *headers, const char *name)
{
    // Assertions for debugging
    assert(headers != NULL);
    assert(name != NULL);

    if (strlen(name) + 1 > MAX_HEADER_BUFFER_SIZE) {
        errno = EINVAL;
        return NULL;
    }
    uint16_t words_count = 0;
    for (int i = 0; i < headers->size; i++) {
        // case insensitive name comparison of headers
        if (words_count%2 == 0 && (strcasecmp(name, headers->buffer + i) == 0)) {
            // found a header with the same name
            DEBUG("Found header with name '%s'", name);
            return (headers->buffer + i + 1 + strlen(name));
        }
        //now move forward the length of the string
        i += strlen(headers->buffer + i);
        words_count++;
    }
    return NULL;
}

/*
 * Function: headers_new
 * Main function to add new header into the header list
 * To add with replacement is overwrite the value of the header if it exist
 * To add without replacement is to concatenate previous value
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 *      -> with_replacement: boolean value to indicate if it is added with replacement or not
 * Output:
 *      0 if success, -1 if error
 */
int headers_new(headers_t *headers, const char *name, const char *value, uint8_t with_replacement)
{

    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    uint16_t name_len = strlen(name);
    uint16_t value_len = strlen(value);
/*
 * Function: headers_add
 * Add new header using headers_new function without replacement
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      0 if success, -1 if error
 */

    if (name_len > MAX_HEADER_NAME_LEN || value_len > MAX_HEADER_VALUE_LEN){
        errno = EINVAL;
        return -1;
    }

    if (name_len + 1 > MAX_HEADER_BUFFER_SIZE || value_len + 1 > MAX_HEADER_BUFFER_SIZE) {
        errno = EINVAL;
        return -1;
    }

    
    char *prev_value = headers_get(headers, name);

    if(prev_value != NULL){
    //case in which already exist entry with same name
        //plus 1 in case without replacement is for separation mark ','
        if((!with_replacement && (value_len + 1 + headers->size > MAX_HEADER_BUFFER_SIZE))
            || (with_replacement && (headers->size - strlen(prev_value) + value_len > MAX_HEADER_BUFFER_SIZE))){
            errno = ENOMEM;
            return -1;
        }

        char buffer_aux[MAX_HEADER_BUFFER_SIZE];
        memset(buffer_aux, 0, sizeof(buffer_aux));
        
        uint16_t i_aux = 0;
        uint16_t entries_counter = 0;

        for (uint16_t i = 0; i < MAX_HEADER_BUFFER_SIZE; i++) {
            
            entries_counter++;

            // case insensitive name comparison of headers
            if (strncasecmp(headers->buffer + i, name, name_len) == 0) {
                
                DEBUG("Found header with name: '%s'", name);

                //First cpy name into aux
                strncpy(buffer_aux + i_aux , headers->buffer + i, name_len + 1);
                i_aux += name_len + 1;
                i += name_len + 1;
                //Then previous value
                uint16_t previous_value_len = strlen(headers->buffer + i);

                if(!with_replacement){
                    //case headers_add
                    strncpy(buffer_aux + i_aux , headers->buffer + i, previous_value_len);
                    i_aux += previous_value_len;
                    i += previous_value_len;
                    //Add separation mark
                    buffer_aux[i_aux] = ','; 
                    i_aux++;
                    //Finally copy new value
                    strncpy(buffer_aux + i_aux, value, value_len + 1);
                    i_aux += value_len + 1;
                    //+1 is for the comma
                    headers->size += (value_len + 1);

                } else {
                    //case headers_set, forget previous value
                    strncpy(buffer_aux + i_aux, value, value_len + 1);
                    i += previous_value_len;
                    i_aux += value_len + 1;
                    headers->size -= previous_value_len;
                    headers->size += value_len;
                }

            } else {

                //Cpy name
                uint16_t previous_name_len = strlen(headers->buffer + i);
                strncpy(buffer_aux + i_aux, headers->buffer + i, previous_name_len + 1);
                i_aux += (previous_name_len + 1); 
                i += (previous_name_len + 1);
                //Then cpy value
                uint16_t previous_value_len = strlen(headers->buffer + i);
                strncpy(buffer_aux + i_aux, headers->buffer + i, previous_value_len + 1);
                i_aux += (previous_value_len + 1);
                i += previous_value_len;
            }

                if(entries_counter >= headers->n_entries) break;

        }

        //Finally copy all from aux buffer to original one
        memcpy(headers->buffer, buffer_aux, MAX_HEADER_BUFFER_SIZE);

    } else {
    //case no dupĺicate

        // + 2 is for the two 0's of EOS
        if (headers->size + name_len + value_len + 2 > MAX_HEADER_BUFFER_SIZE) {
            errno = ENOMEM;
            return -1;
        }
        
        // Set header name
        strncpy(headers->buffer + headers->size, name, name_len + 1);

        // Set header value
        strncpy(headers->buffer + headers->size + name_len + 1, value, value_len + 1);

        DEBUG("Wrote header: '%s':'%s'", headers->buffer + headers->size, headers->buffer + headers->size + name_len + 1);

        // Update header count
        headers->size += (name_len + value_len + 2);
        headers->n_entries ++;
    }

    return 0;
}

/*
 * Function: headers_add
 * Add new header using headers_new function without replacement
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      0 if success, -1 if error
 */
int headers_add(headers_t *headers, const char *name, const char *value){
    return headers_new(headers, name, value, 0);
}

/*
 * Function: headers_set
 * Add new header using headers_new function without replacement
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      0 if success, -1 if error
 */
int headers_set(headers_t *headers, const char *name, const char *value)
{
    return headers_new(headers, name, value, 1);
}

/*
 * Function: headers_count
 * Returns the amount of entries in the header list
 * Input:
 *      -> *headers: header list
 * Output:
 *      Amount of entries in the header list
 */
int headers_count(headers_t *headers)
{   
    return headers->n_entries;
}

/*
 * Function: headers_get_name_from_index
 * Returns the name of the header of a certain index in the list
 * Input:
 *      -> *headers: header list
 *      -> index: index of the header list to search
 * Output:
 *      String with the name of the header, NULL if it doesn't exist
 */
char *headers_get_name_from_index(headers_t *headers, int index)
{
    // Assertions for debugging
    assert(headers != NULL);

    if(index > headers->n_entries - 1){
        return NULL;
    }
    
    uint16_t words_count=0;
    uint16_t entries_count=0;
    
    // Every two words is an entry
    for(uint16_t i=0; i<headers->size; i++){
        // Case find header of exact index
        if(words_count%2 == 0 && index == entries_count){
            return headers->buffer + i;
        }
        // Else advance the len of the string
        i+=strlen(headers->buffer + i);
        words_count++;
        
        if(words_count%2==0) entries_count++;

    }
    return NULL;

}

/*
 * Function: headers_get_value_from_index
 * Returns the value of the header of a certain index in the list
 * Input:
 *      -> *headers: header list
 *      -> index: index of the header list to search
 * Output:
 *      String with the value of the header, NULL if it doesn't exist
 */
char *headers_get_value_from_index(headers_t *headers, int index)
{
    char *name_pos = headers_get_name_from_index(headers, index);
    return name_pos + strlen(name_pos) + 1;
}

/*
 * Function: headers_get_all 
 * Copy pointers of name and value of each header in the header list to a header array
 * Input:
 *      -> *headers: header list struct
 *      -> *headers_array: array of pointers to headers name and value.
 * Output:
 *      (void)
 */
void headers_get_all(header_list_t* headers, header_t *headers_array){
    
    assert(headers != NULL);
    uint16_t n_entries = 0;

    for(int i=0; i<headers->size; i++){
        //for every header

        //get name
        char *name = headers->buffer + i;
        headers_array[n_entries].name = name;
        //plus one for 0 between them
        i += strlen(name) + 1;
        //get value 
        char *value = headers->buffer + i;
        headers_array[n_entries].value = value;
        i += strlen(value);
        n_entries ++;
    }
}


/*
 * Function: headers_validate
 * Check if the header list is correct, which means, every header in it is correct
 * 
 * There shouldn't be any empty values nor duplicated ones
 * 
 * Input:
 *      -> *headers: header list
 * Output:
 *      0 if correct, -1 otherwise
 */
int headers_validate(headers_t* headers) {
    // Assertions for debugging
    assert(headers != NULL);

    header_t headers_array[headers_count(headers)];
    headers_get_all(headers, headers_array);

    for (int i = 0; i < headers_count(headers); i++) {
        char* name = headers_array[i].name;
        char* value = headers_array[i].value;
        if (value == NULL)
        {
            ERROR("Headers validation failed, the \"%s\" field had no value", name);
            return -1;
        }
        if (strchr(value, ',') != NULL)
        {
            ERROR("Headers validation failed, the \"%s\" field was ducplicated", name);
            return -1;
        }
    }

    return 0;
}


/*
 * Function: headers_get_header_list_size
 * Returns the size of the header list related to the RFC Standards
 * Input:
 *      -> *headers: header list
 * Output:
 *      Size of the header list in bytes
 */
uint32_t headers_get_header_list_size(headers_t *headers)
{
    uint32_t header_list_size = headers->size;
    // minus all the 0's to end string
    header_list_size -= 2*(headers->n_entries);
    // plus an overhead of 32 octects for each string, name and value
    header_list_size += 64*(headers->n_entries);

    return header_list_size;    //OK
}
