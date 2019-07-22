#ifndef HEADERS_H
#define HEADERS_H

#ifndef CONF_MAX_HEADER_COUNT
#define MAX_HEADER_COUNT (16)
#else
#define MAX_HEADER_COUNT (CONF_MAX_HEADER_COUNT)
#endif

#ifndef CONF_MAX_HEADER_NAME_SIZE
#define MAX_HEADER_NAME_SIZE (32)
#else
#define MAX_HEADER_NAME_SIZE (CONF_MAX_HEADER_NAME_SIZE)
#endif

#ifndef CONF_MAX_HEADER_VALUE_SIZE
#define MAX_HEADER_VALUE_SIZE (32)
#else
#define MAX_HEADER_VALUE_SIZE (CONF_MAX_HEADER_VALUE_SIZE)
#endif


typedef struct {
    char name[MAX_HEADER_NAME_SIZE];
    char value[MAX_HEADER_VALUE_SIZE];
} header_t;



#endif
