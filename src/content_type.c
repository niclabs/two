#include <string.h>
#include <strings.h>

#include "content_type.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"

char *allowed_content_types[] = CONTENT_TYPES;

#define CONTENT_TYPES_LEN                                                      \
    (sizeof(allowed_content_types) / sizeof(*allowed_content_types))

char *content_type_allowed(char *content_type)
{
    if (content_type == NULL) {
        return NULL;
    }

    for (unsigned int i = 0; i < CONTENT_TYPES_LEN; i++) {
        int len = strlen(allowed_content_types[i]);
        if (strncmp(content_type, allowed_content_types[i], len) == 0) {
            return allowed_content_types[i];
        }
    }
    return NULL;
}
