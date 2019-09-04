#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "headers.h"
#include "logging.h"

int headers_init(headers_t *headers, header_t *hlist, int maxlen)
{
    memset(headers, 0, sizeof(*headers));
    memset(hlist, 0, maxlen * sizeof(*hlist));


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

    for (int i = 0; i < headers->count; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->headers[i].name, name, MAX_HEADER_NAME_LEN) == 0) {
            DEBUG("Found header with name: '%s'", name);

            if (strlen(value) > MAX_HEADER_VALUE_LEN - strlen(headers->headers[i].value + 1)) {
                errno = ENOMEM;
                return -1;
            }

            // Concatenate values
            strncat(headers->headers[i].value, ",", MAX_HEADER_VALUE_LEN);
            strncat(headers->headers[i].value, value, MAX_HEADER_VALUE_LEN);

            return 0;
        }
    }

    if (headers->count >= headers->maxlen) {
        errno = ENOMEM;
        return -1;
    }

    // Set header name
    strncpy(headers->headers[headers->count].name, name, MAX_HEADER_NAME_LEN - 1);

    // Set header value
    strncpy(headers->headers[headers->count].value, value, MAX_HEADER_VALUE_LEN - 1);

    DEBUG("Wrote header: '%s':'%s'", headers->headers[headers->count].name, headers->headers[headers->count].value);

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
            DEBUG("Found header with name: '%s'", name);
            // found a header with the same name
            strncpy(headers->headers[i].value, value, MAX_HEADER_VALUE_LEN);
            DEBUG("Wrote header: '%s':'%s'", headers->headers[i].name, headers->headers[i].value);
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

    DEBUG("Wrote header: '%s':'%s'", headers->headers[headers->count].name, headers->headers[headers->count].value);

    // Update header count
    headers->count++;

    return 0;
}

char *headers_get(headers_t *headers, const char *name)
{
    // Assertions for debugging
    assert(headers != NULL);
    assert(name != NULL);

    if (strlen(name) > MAX_HEADER_NAME_LEN) {
        errno = EINVAL;
        return NULL;
    }

    for (int i = 0; i < headers->count; i++) {
        // case insensitive name comparison of headers
        if (strncasecmp(headers->headers[i].name, name, MAX_HEADER_NAME_LEN) == 0) {
            // found a header with the same name
            DEBUG("Found header with name '%s'", name);
            return headers->headers[i].value;
        }
    }
    return NULL;
}


int headers_validate(headers_t* headers) {
    // Assertions for debugging
    assert(headers != NULL);

    for (int i = 0; i < headers_count(headers); i++) {
        char* name = headers_get_name_from_index(headers, i);
        char* value = headers_get_value_from_index(headers, i);
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


int headers_count(headers_t *headers)
{
    return headers->count;
}

header_t *get_header_from_index(headers_t *headers, int index)
{
    // Assertions for debugging
    assert(headers != NULL);
    assert(index >= 0);
    assert(index < headers->maxlen);

    return &headers->headers[index];
}

char *headers_get_name_from_index(headers_t *headers, int index)
{
    return get_header_from_index(headers, index)->name;
}

char *headers_get_value_from_index(headers_t *headers, int index)
{
    return get_header_from_index(headers, index)->value;
}

uint32_t headers_get_header_list_size(headers_t *headers)
{
    int header_list_size = 0;

    for (int i = 0; i < headers->count; i++) {
        header_list_size += strlen(headers->headers[i].name);
        header_list_size += strlen(headers->headers[i].value);
        header_list_size += 64; //overhead of 32 octets for each, name and value
    }
    return header_list_size;    //OK
}
