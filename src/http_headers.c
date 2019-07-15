#include <strings.h>
#include <string.h>
#include <errno.h>

#include "http_headers.h"


int http_headers_init(http_headers_t *headers)
{
    memset(headers, 0, sizeof(*headers));
    return 0;
}

int http_headers_set_header(http_headers_t *headers, const char *name, const char *value)
{
    if (headers == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(name) > HTTP_MAX_HEADER_NAME_LEN || strlen(value) > HTTP_MAX_HEADER_VALUE_LEN) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < headers->size; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->list[i].name, name, HTTP_MAX_HEADER_NAME_LEN) == 0) {
            // found a header with the same name
            strncpy(headers->list[i].value, value, HTTP_MAX_HEADER_VALUE_LEN);
            return 0;
        }
    }

    if (headers->size >= HTTP_MAX_HEADERS_SIZE) {
        errno = ENOMEM;
        return -1;
    }

    // Set header name
    strncpy(headers->list[headers->size].name, name, HTTP_MAX_HEADER_NAME_LEN);

    // Set header value
    strncpy(headers->list[headers->size].value, value, HTTP_MAX_HEADER_VALUE_LEN);

    // Update header count
    headers->size++;

    return 0;
}

int http_headers_get_header(http_headers_t *headers, const char *name, char *value)
{
    if (headers == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(name) > HTTP_MAX_HEADER_NAME_LEN) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < headers->size; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->list[i].name, name, HTTP_MAX_HEADER_NAME_LEN) == 0) {
            // found a header with the same name
            strncpy(value, headers->list[i].value, HTTP_MAX_HEADER_VALUE_LEN);
            return 0;
        }
    }
    return -1;
}


int http_headers_size(http_headers_t *headers)
{
    return headers->size;
}
