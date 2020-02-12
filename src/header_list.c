#include <assert.h>
#include <string.h>
#include <strings.h>

#include "header_list.h"
#include "logging.h"
#include "macros.h"

// Use this if strnlen is missing.
size_t strnlen(const char *str, size_t max)
{
    const char *end = memchr(str, 0, max);
    return end ? (size_t)(end - str) : max;
}

/*
 * Function: headers_init
 * Initializes a headers struct
 * Input:
 *      -> *headers: struct which will be initialized
 * Output:
 *      (void)
 */
void header_list_reset(header_list_t *headers)
{
    assert(headers != NULL);

    memset(headers->buffer, 0, HEADER_LIST_MAX_SIZE);

    headers->count = 0;
    headers->size  = 0;
}

// return the position in the buffer of the given name
int header_list_index(header_list_t *headers, const char *name)
{
    assert(headers != NULL);
    assert(name != NULL);

    int pos   = 0;
    char *ptr = headers->buffer;
    while (pos < headers->size) {
        int len = strnlen(ptr, headers->size - pos);
        if (strncasecmp(name, ptr, len) == 0) {
            return pos;
        }

        // increase the counter
        pos += len + 1;
        ptr += len + 1;

        // skip the value
        while (*ptr != 0 && pos < headers->size) {
            ++pos;
            ++ptr;
        }

        // skip padding
        while (*ptr == 0 && pos < headers->size) {
            ++pos;
            ++ptr;
        }
    }
    return -1;
}

// remove a portion of the array memory
void header_list_splice(header_list_t *headers, unsigned int start,
                        unsigned int count)
{
    char *dst = headers->buffer + start;
    char *src = headers->buffer + start + count;

    // shift the buffer to the left
    memmove(dst, src, headers->size - start - count);

    // update size
    headers->size -= count;
}

/*
 * Function: header_list_get
 * Get header value of the header list
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      String with the value of the header, NULL if it doesn't exist
 */
char *header_list_get(header_list_t *headers, const char *name)
{
    int pos = header_list_index(headers, name);
    if (pos < 0) {
        return NULL;
    }

    char *ptr = headers->buffer + pos;
    int len   = strnlen(ptr, headers->size - pos);
    return ptr + len + 1;
}

int header_list_append(header_list_t *headers, const char *name,
                       const char *value)
{
    unsigned int nlen = strlen(name);
    unsigned int vlen = strlen(value);

    // not enough space left in the header list to append the new value
    if (nlen + 1 + vlen + 1 >
        (unsigned)(HEADER_LIST_MAX_SIZE - headers->size)) {
        return -1;
    }

    char *ptr = headers->buffer + headers->size;

    memcpy(ptr, name, nlen);
    ptr[nlen] = 0;
    ptr += nlen + 1;

    memcpy(ptr, value, vlen);
    ptr[vlen] = 0;
    ptr += vlen + 1;

    // update size
    headers->size += nlen + 1 + vlen + 1;

    // add a new entry
    headers->count += 1;

    return 0;
}

/*
 * Function: header_list_add
 * Add new header using headers_new function without replacement
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      0 if success, -1 if error
 */
int header_list_add(header_list_t *headers, const char *name, const char *value)
{
    assert(value != NULL);

    // go to the position for the name
    int start = header_list_index(headers, name);
    if (start < 0) {
        return header_list_append(headers, name, value);
    }

    int end   = start;
    char *ptr = headers->buffer + start;

    int nlen = strnlen(ptr, headers->size - end);

    // skip the name
    end += nlen + 1;
    ptr += nlen + 1;

    // get value and its length
    char *v  = ptr;
    int vlen = strnlen(ptr, headers->size - end);

    // update the pointer
    ptr += vlen + 1;
    end += vlen + 1;

    // if ',' + value + '\0' does not fit in the memory, return -1
    if (1 + strlen(value) > (unsigned)(HEADER_LIST_MAX_SIZE - headers->size)) {
        return -1;
    }

    // otherwise splice the existing name:value and reinsert
    int newlen = vlen + 1 + strlen(value);
    char newvalue[newlen + 1];

    // copy the old value
    memcpy(newvalue, v, vlen);

    // add a separating ','
    newvalue[vlen] = ',';

    // copy the new value
    memcpy(newvalue + vlen + 1, value, strlen(value));
    newvalue[newlen] = 0;

    // splice the memory from the array
    header_list_splice(headers, start, end - start);
    headers->count -= 1;

    return header_list_append(headers, name, newvalue);
}

/*
 * Function: header_list_set
 * Add new header using headers_new function without replacement
 * Input:
 *      -> *headers: header list
 *      -> *name: name of the header to search
 * Output:
 *      0 if success, -1 if error
 */
int header_list_set(header_list_t *headers, const char *name, const char *value)
{
    assert(value != NULL);

    // go to the position for the name
    int start = header_list_index(headers, name);
    if (start < 0) {
        return header_list_append(headers, name, value);
    }

    int end   = start;
    char *ptr = headers->buffer + start;

    int nlen = strnlen(ptr, headers->size - end);

    // skip the name
    end += nlen + 1;
    ptr += nlen + 1;

    // get value length
    int vlen = strnlen(ptr, headers->size - end);

    // update the pointer
    ptr += vlen + 1;
    end += vlen + 1;

    // if ',' + value + '\0' does not fit in the memory, return -1
    if (1 + strlen(value) + 1 >
        (unsigned)(HEADER_LIST_MAX_SIZE - headers->size - vlen)) {
        return -1;
    }

    // splice the memory from the array
    header_list_splice(headers, start, end - start);
    headers->count -= 1;

    return header_list_append(headers, name, value);
}

/*
 * Function: header_list_count
 * Returns the amount of entries in the header list
 * Input:
 *      -> *headers: header list
 * Output:
 *      Amount of entries in the header list
 */
unsigned int header_list_count(header_list_t *headers)
{
    return headers->count;
}

/*
 * Function: header_list_size
 * Returns the size in bytes of the header list
 * Input:
 *      -> *headers: header list
 * Output: Size in bytes of in the header list
 */
unsigned int header_list_size(header_list_t *headers)
{
    return headers->size;
}

http_header_t *header_list_all(header_list_t *headers, http_header_t *hlist)
{
    assert(headers != NULL);

    int pos   = 0;
    char *ptr = headers->buffer;
    int index = 0;

    while (pos < headers->size) {
        int len = strnlen(ptr, headers->size - pos);
        if (len == 0) {
            return hlist;
        }

        // set the name pointer
        hlist[index].name = ptr;

        // increase the counter
        pos += len + 1;
        ptr += len + 1;

        // set the value pointer
        hlist[index].value = ptr;

        // skip the value
        while (*ptr != 0 && pos < headers->size) {
            ++pos;
            ++ptr;
        }

        // skip padding
        while (*ptr == 0 && pos < headers->size) {
            ++pos;
            ++ptr;
        }

        // update count
        index += 1;
    }
    return hlist;
}
