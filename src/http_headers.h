/**
 * Set of functions to handle http headers
 */

#ifndef HTTP_HEADERS_H
#define HTTP_HEADERS_H

#include <stdint.h>

#ifdef HTTP_CONF_MAX_HEADER_NAME_SIZE
#define HTTP_MAX_HEADER_NAME_SIZE HTTP_CONF_MAX_HEADER_NAME_SIZE
#else
#define HTTP_MAX_HEADER_NAME_SIZE 32
#endif

#ifdef HTTP_CONF_MAX_HEADER_VALUE_SIZE
#define HTTP_MAX_HEADER_VALUE_SIZE HTTP_CONF_MAX_HEADER_VALUE_SIZE
#else
#define HTTP_MAX_HEADER_VALUE_SIZE 128
#endif

#ifdef HTTP_CONF_MAX_HEADER_COUNT
#define HTTP_MAX_HEADER_COUNT HTTP_CONF_MAX_HEADER_COUNT
#else
#define HTTP_MAX_HEADER_COUNT 32
#endif


typedef struct {
    char name[HTTP_MAX_HEADER_NAME_SIZE];
    char value[HTTP_MAX_HEADER_VALUE_SIZE];
} http_header_t;

typedef struct {
    http_header_t list[HTTP_MAX_HEADER_COUNT];
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

#endif
