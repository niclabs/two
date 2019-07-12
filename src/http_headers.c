#include <strings.h>
#include <string.h>
#include <errno.h>

#include "http_headers.h"



int http_headers_init(http_headers_t *headers)
{
    memset(headers, 0, sizeof(http_headers_t));
}

int http_headers_set_header(http_headers_t *headers, const char *name, const char *value)
{
    if (headers == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(name) >= HTTP_MAX_HEADER_NAME_SIZE || strlen(value) >= HTTP_MAX_HEADER_VALUE_SIZE) {
        errno = EINVAL;
        return -1;
    }

    if (headers->size >= HTTP_MAX_HEADER_COUNT) {
        errno = ENOMEM;
        return -1;
    }

    for (int i = 0; i < headers->size; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->list[i].name, name, HTTP_MAX_HEADER_NAME_SIZE) == 0) {
            // found a header with the same name
            int len = strlen(value);
            memcpy(headers->list[i].value, value, len);
            if (len < HTTP_MAX_HEADER_VALUE_SIZE) {
                // set the rest of the memory to 0
                memset(headers->list[i].value + len, 0, HTTP_MAX_HEADER_VALUE_SIZE - len);
            }
            return 0;
        }
    }

    int len;


    // Set header name
    len = strlen(name);
    memcpy(headers->list[headers->size].name, name, len);
    if (len < HTTP_MAX_HEADER_NAME_SIZE) {
        memset(headers->list[headers->size].name + len, 0, HTTP_MAX_HEADER_NAME_SIZE - len);
    }

    // Set header value
    len = strlen(value);
    memcpy(headers->list[headers->size].value, value, len);
    if (len < HTTP_MAX_HEADER_VALUE_SIZE) {
        memset(headers->list[headers->size].value + len, 0, HTTP_MAX_HEADER_VALUE_SIZE - len);
    }

    return 0;
}
