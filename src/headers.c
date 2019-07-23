#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "headers.h"
#include "logging.h"

int headers_init(headers_t *headers, header_t *hlist, int maxlen)
{
    memset(headers, 0, sizeof(*headers));
    int rc = memset(hlist, 0, maxlen * sizeof(*hlist));
    ERROR("memset returning %d", rc);
    if (rc <= 0){
        ERROR("memset returning %d", rc);
        return -1;

    }

    headers->headers = hlist;
    headers->maxlen = maxlen;
    headers->count = 0;
    return 0;
}

int headers_add(headers_t *headers, const char *name, const char *value)
{
    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    if (strlen(name) > MAX_HEADER_NAME_LEN || strlen(value) > MAX_HEADER_VALUE_LEN) {
        errno = EINVAL;
        return -1;
    }

    if (headers->count >= headers->maxlen) {
        errno = ENOMEM;
        return -1;
    }

    // Set header name
    strncpy(headers->headers[headers->count].name, name, MAX_HEADER_NAME_LEN - 1);

    // Set header value
    strncpy(headers->headers[headers->count].value, value, MAX_HEADER_VALUE_LEN - 1);

    // Update header count
    headers->count++;

    return 0;

}

int headers_set(headers_t *headers, const char *name, const char *value)
{
    // Assertions for debugging
    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    // TODO: should we check value len or just write the truncated value?
    if (strlen(name) > MAX_HEADER_NAME_LEN || strlen(value) > MAX_HEADER_VALUE_LEN) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < headers->count; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->headers[i].name, name, MAX_HEADER_NAME_LEN) == 0) {
            DEBUG("Found '%s'", name);
            // found a header with the same name
            strncpy(headers->headers[i].value, value, MAX_HEADER_VALUE_LEN);
            DEBUG("Wrote: '%s'", headers->headers[i].value);
            return 0;
        }
    }

    if (headers->count >= headers->maxlen) {
        errno = ENOMEM;
        return -1;
    }

    // Set header name
    strncpy(headers->headers[headers->count].name, name, MAX_HEADER_NAME_LEN);

    // Set header value
    strncpy(headers->headers[headers->count].value, value, MAX_HEADER_VALUE_LEN);

    DEBUG("Wrote: '%s':'%s'", headers->headers[headers->count].name, headers->headers[headers->count].value);

    // Update header count
    headers->count++;

    return 0;
}

int headers_get(headers_t *headers, const char *name, char *value)
{
    return headers_get_len(headers, name, value, MAX_HEADER_VALUE_LEN);
}

int headers_get_len(headers_t *headers, const char *name, char *value, size_t len)
{
    // Assertions for debugging
    assert(headers != NULL);
    assert(name != NULL);
    assert(value != NULL);

    if (strlen(name) > MAX_HEADER_NAME_LEN) {
        errno = EINVAL;
        return -1;
    }

    for (int i = 0; i < headers->count; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->headers[i].name, name, MAX_HEADER_NAME_LEN) == 0) {
            // found a header with the same name
            DEBUG("Found '%s'", name);
            strncpy(value, headers->headers[i].value, len);
            value[len] = '\0';
            DEBUG("Read: '%s'", value);
            return 0;
        }
    }
    return -1;
}


int headers_count(headers_t *headers)
{
    return headers->count;
}
