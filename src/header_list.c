#include <strings.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

#include "header_list.h"

#include "config.h"

#undef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"

#undef MIN
#define MIN(a, b) ((a) < (b)) ? (a) : (b)


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
    headers->size = 0;
}

// return the position in the buffer of the given name
int header_list_find(header_list_t *headers, const char *name)
{
    assert(headers != NULL);
    assert(name != NULL);

    int pos = 0;
    char *ptr = headers->buffer;
    while (pos < headers->size) {
        int len = strnlen(ptr, headers->size - pos);

        if (len == 0 || strncasecmp(name, ptr, len) == 0) {
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
    return pos;
}

// remove a portion of the array memory
void header_list_splice(header_list_t *headers, unsigned int start, unsigned int count)
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
    int pos = header_list_find(headers, name);
    char *ptr = headers->buffer + pos;
    int len = strnlen(ptr, headers->size - pos);

    if (len > 0 && strncasecmp(name, ptr, len) == 0) {
        return ptr + len + 1;
    }
    return NULL;
}

int header_list_append(header_list_t *headers, const char *name, const char *value)
{
    unsigned int nlen = strlen(name);
    unsigned int vlen = strlen(value);

    // not enough space left in the header list to append the new value
    if (nlen + 1 + vlen + 1 >= (unsigned)(HEADER_LIST_MAX_SIZE - headers->size)) {
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

    // append padding bytes if possible
    unsigned int padding = MIN(HEADER_LIST_PADDING, HEADER_LIST_MAX_SIZE - headers->size);
    memset(ptr, 0, padding);
    headers->size += padding;

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
    int start = header_list_find(headers, name);
    int end = start;


    char *ptr = headers->buffer + start;
    int len = strnlen(ptr, headers->size - end);

    if (len > 0 && strncasecmp(name, ptr, len) == 0) {
        // skip the name
        end += len + 1;
        ptr += len + 1;

        // get value and its length
        char *v = ptr;
        len = strnlen(ptr, headers->size - end);

        // update the pointer
        ptr += len;
        end += len;

        // get padding zeroes
        unsigned int padding = 0;
        while (*ptr == 0 && end < headers->size) {
            end++;
            ptr++;
            padding++;
        }

        // if enough empty bytes to append value (+separating comma + ending zero)
        // append immediately
        if (strlen(value) + 1 + 1 < padding) {
            v[len] = ',';

            // append the value at the end
            strncpy(v + len + 1, value, padding - 1);

            return 0;
        }
        // else if there are enough bytes left in the array for the new value
        // name + '\0' + existing_value + ',' + value + '\0' has to fit in the remainging array
        else if (strlen(value) + 1 + 1 < (unsigned)(HEADER_LIST_MAX_SIZE - headers->size + padding)) {
            // Create a new value
            int newlen = len + 1 + strlen(value);
            char newvalue[newlen + 1];
            memcpy(newvalue, v, len);
            newvalue[len] = ',';
            memcpy(newvalue + len + 1, value, strlen(value));
            newvalue[newlen] = 0;

            // splice the memory from the array
            header_list_splice(headers, start, end - start);

            // update the number of entries
            headers->count -= 1;

            // append name and value at the end
            return header_list_append(headers, name, newvalue);
        }
        else {
            return -1;
        }
    }

    // if we are here append at the end of the array
    return header_list_append(headers, name, value);
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
    int start = header_list_find(headers, name);
    int end = start;

    char *ptr = headers->buffer + start;
    int len = strnlen(ptr, headers->size - end);

    if (len > 0 && strncasecmp(name, ptr, len) == 0) {
        // skip the name
        end += len + 1;
        ptr += len + 1;

        // get value and its length
        char *v = ptr;
        len = strnlen(ptr, headers->size - end);

        // update the pointer
        ptr += len;
        end += len;

        // get padding zeroes
        unsigned int padding = 0;
        while (*ptr == 0 && end < headers->size) {
            end++;
            ptr++;
            padding++;
        }

        // if enough empty bytes to fit the new value
        // replace
        unsigned int newlen = strlen(value);
        unsigned int available = len + padding;
        if (newlen + 1 < available) {
            // replace the value
            memcpy(v, value, available - 1);
            
            // set the remaining memory to zero
            memset(v + newlen, 0, available - newlen);

            // compress padding if larger than the one defined
            // by the constant
            if (available - newlen > HEADER_LIST_PADDING + 1) {
                unsigned int newend = newlen + 1 + HEADER_LIST_PADDING;
                memmove(v + newend, v + newlen + padding, headers->size - end);
                headers->size -= (available - newlen) - (HEADER_LIST_PADDING + 1);
            }

            return 0;
        }
        // else if there are enough bytes left in the array for the new value
        // splice the array and append at the end
        else if (strlen(value) + 1 < (unsigned)(HEADER_LIST_MAX_SIZE - headers->size + len + padding)) {
            // splice the memory from the array
            header_list_splice(headers, start, end - start);
            
            // update the number of entries
            headers->count -= 1;

            // append name and value at the end
            return header_list_append(headers, name, value);
        }
        else {
            return -1;
        }
    }

    // if we are here append at the end of the array
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

http_header_t * header_list_all(header_list_t *headers, http_header_t * hlist) {
    assert(headers != NULL);

    int pos = 0;
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
