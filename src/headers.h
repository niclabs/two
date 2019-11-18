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
 * Data structure to indicate location of a header
 * Uses pointer to header buffer
 */
typedef struct {
    char *name;
    char *value;
} header_t;

/**
 * Initialize header list with specified array and set list counter to zero
 * This function will reset provided memory
 *
 * @param headers headers data structure
 * @param hlist h pointer to header_t list
 * @return (void) 
 */
void headers_init(headers_t * headers);

/**
 *This function will reset header list
 *
 * @param headers headers data structure
 * @return (void)
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
 * @return value header value of the header with name 'name' or NULL if error
 * */
char * headers_get(headers_t * headers, const char * name);

/**
 * Checks all headers for validity.
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
/**
 * Copy pointers to headers into a array of header_t
 * This makes iterating through headers easier
 * 
 * @param headers headers list data structure
 * @param headers_array header array that stores name and value of each ones as pointers to the buffer
 * 
 * @return (void)
 */
void headers_get_all(header_list_t* headers, header_t *headers_array);


#endif
