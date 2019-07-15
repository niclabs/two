/**
 * Set of functions to handle http headers
 */

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <stdint.h>

#ifdef HTTP_CONF_MAX_HEADER_NAME_LEN
#define HTTP_MAX_HEADER_NAME_LEN HTTP_CONF_MAX_HEADER_NAME_LEN
#else
#define HTTP_MAX_HEADER_NAME_LEN 32
#endif

#ifdef HTTP_CONF_MAX_HEADER_VALUE_LEN
#define HTTP_MAX_HEADER_VALUE_LEN HTTP_CONF_MAX_HEADER_VALUE_LEN
#else
#define HTTP_MAX_HEADER_VALUE_LEN 128
#endif

#ifdef HTTP_CONF_MAX_HEADERS_SIZE
#define HTTP_MAX_HEADERS_SIZE HTTP_CONF_MAX_HEADERS_SIZE
#else
#define HTTP_MAX_HEADERS_SIZE 32
#endif


typedef struct {
    char name[HTTP_MAX_HEADER_NAME_LEN];
    char zero; // prevent overflow of name in printf
    char value[HTTP_MAX_HEADER_VALUE_LEN];
} http_header_t;

typedef struct {
    http_header_t list[HTTP_MAX_HEADERS_SIZE];
    uint8_t size;
} http_headers_t;

/**
 * Initialize memory for headers
 */
int http_headers_init(http_headers_t * headers);

/**
 * Update header value in the given headers data structure
 *
 * @return 0 if header was set successfully or -1 if error
 */
int http_headers_set_header(http_headers_t * headers, const char * name, const char * value);

/**
 * Retrieve header with name name from the headers data structure and set the value in the value pointer
 *
 * @return 0 if header was found or -1 if not found
 */
int http_headers_get_header(http_headers_t * headers, const char * name, char * value);

/**
 * Return the number of stored headers in the header list
 */
int http_headers_size(http_headers_t * headers);

#endif
