#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>
#include <stdint.h>

#ifdef CONF_MAX_HEADER_BUFFER_SIZE
#define MAX_HEADER_BUFFER_SIZE (CONF_MAX_HEADER_BUFFER_SIZE)
#else
#define MAX_HEADER_BUFFER_SIZE (4096)
#endif

#ifndef CONF_MAX_HEADER_NAME_LEN
#define MAX_HEADER_NAME_LEN (16)
#else
#define MAX_HEADER_NAME_LEN (CONF_MAX_HEADER_NAME_LEN)
#endif

#ifndef CONF_MAX_HEADER_VALUE_LEN
#define MAX_HEADER_VALUE_LEN (32)
#else
#define MAX_HEADER_VALUE_LEN (CONF_MAX_HEADER_VALUE_LEN)
#endif



/**
 * Data structure to store a header list
 *
 * Should not be used directly, use provided API methods
 */
typedef struct {
    char buffer[MAX_HEADER_BUFFER_SIZE];
    uint16_t n_entries;
    uint16_t size;
} headers_t;


/**
 * Initialize header list with specified array and set list counter to zero
 * This function will reset provided memory
 *
 * @param headers headers data structure
 * @param hlist h pointer to header_t list of size maxlen
 * @param maxlen maximum number of elements that the header list can store
 * @return 0 if ok -1 if an error ocurred
 */
void headers_init(headers_t * headers);

/**
 *This function will reset header list
 *
 * @param headers headers data structure
 * @return 0 if ok -1 if an error ocurred
 */
void headers_clean(headers_t *headers);

/**
 * Add a header to the list, if header already exists, concatenate value
 * with a ',' as specified in
 * https://www.w3.org/Protocols/rfc2616/rfc2616-sec4.html#sec4.2
 *
 * @param headers headers data structure
 * @param name header name
 * @param value header value
 * @return 0 if ok -1 if an error ocurred
 */
int headers_add(headers_t * headers, const char * name, const char * value);

/**
 * Set the header for a given name, if the header is already set
 * it replaces the value with the new given value
 *
 * @param headers headers data structure
 * @param name header name
 * @param value header value
 * @return 0 if ok -1 if an error ocurred
 */
int headers_set(headers_t * headers, const char * name, const char * value);

/**
 * Get a pointere to the value of the header with name 'name'
 *
 * @param headers header list data structure
 * @param name name of the header to search
 * @param value header value of the header with name 'name' or NULL if error
 * */
char * headers_get(headers_t * headers, const char * name);

/**
 * Cheacks all headers for validity.
 *
 * There shouldn't be any ommited values, nor any duplicated ones.
 *
 * @param headers headers list data structure
 * @return 0 if ok -1 if validation failed
 * */
int headers_validate(headers_t* headers);

/**
 * Return total number of headers in the header list
 */
int headers_count(headers_t * headers);

/**
 * Return header name from header in the position given by index
 */
char * headers_get_name_from_index(headers_t * headers, int index);

/**
 * Return header value from header in the position given by index
 */
char * headers_get_value_from_index(headers_t * headers, int index);

/**
 * Calculate size of header list for http/2
 */
uint32_t headers_get_header_list_size(headers_t* headers);


#endif
