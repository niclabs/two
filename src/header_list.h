#ifndef HEADERS_H
#define HEADERS_H

#include "http.h"
#include "two-conf.h" // library configuration

/**
 * Here a header list is defined as a structure that tries
 * to maximize available memory when storing headers.
 *
 * It is implemented as an array where name-value pairs
 * are separated by zeroes.
 *
 * When an add or set operation is performed on an existing
 * header, the method removes the old name-value pair, splicing
 * the memory, and appends the updated name-value pair at the
 * end of the array.
 *
 * Although moving memory may be expensive, it is expected
 * to happen infrequently, so is a tradeoff taken for implementation
 * simplicity.
 *
 * For the same reason, the header list cannot guarantee that the ordering
 * of insertion will be preserved
 * */

/**
 * Maximum header list size in bytes. It is the value
 * used by http2 MAX_HEADER_LIST_SIZE setting
 */
#ifndef HTTP2_MAX_HEADER_LIST_SIZE
#define HEADER_LIST_MAX_SIZE (512)
#else
#define HEADER_LIST_MAX_SIZE (HTTP2_MAX_HEADER_LIST_SIZE)
#endif

/**
 * Data structure to store a header list
 *
 * Should not be used directly, use provided API methods
 */
typedef struct
{
    char buffer[HEADER_LIST_MAX_SIZE];
    int count;
    int size;
} header_list_t;

/**
 * Data structure to indicate location of a header
 * Uses pointer to header buffer
 */
typedef struct
{
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
void header_list_reset(header_list_t *headers);

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
int header_list_add(header_list_t *headers, const char *name,
                    const char *value);

/**
 * Set the header for a given name, if the header is already set
 * it replaces the value with the new given value
 *
 * @param headers headers data structure
 * @param name header name
 * @param value header value
 * @return 0 if ok -1 if an error ocurred
 */
int header_list_set(header_list_t *headers, const char *name,
                    const char *value);

/**
 * Get a pointere to the value of the header with name 'name'
 *
 * @param headers header list data structure
 * @param name name of the header to search
 * @return value header value of the header with name 'name' or NULL if error
 * */
char *header_list_get(header_list_t *headers, const char *name);

/**
 * Return size of the header list for http/2
 */
unsigned int header_list_size(header_list_t *headers);

/**
 * Return the number of header,value pairs in the list
 */
unsigned int header_list_count(header_list_t *headers);

/**
 * Convert the header list to a an array of http_header_t structs
 *
 * This function simply sets the values for (name, value) pointers inside
 * the to the respective memory location inside the header list. If the header
 * list is modified after this call, the value pointed by the elements in the
 * array WILL change
 */
http_header_t *header_list_all(header_list_t *headers, http_header_t *hlist);

#endif
