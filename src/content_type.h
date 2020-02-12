#ifndef CONTENT_TYPE_H
#define CONTENT_TYPE_H

/**
 * List of allowed content types
 */
#define CONTENT_TYPES                                                          \
    {                                                                          \
        "text/plain", "application/json", "application/cbor"                   \
    }
;

/**
 * Get a pointer to the value of the content type in
 * the content type list or NULL if not found
 *
 * @param content_type value to look for in the allowed list
 * @return pointer to the respective content_type in the list or NULL if not
 * found
 */
char *content_type_allowed(char *content_type);

#endif
