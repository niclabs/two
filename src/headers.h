#ifndef HEADERS_H
#define HEADERS_H

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

typedef struct {
    header_t * headers;
    int count;
    int maxlen;
} headers_t;

int headers_init(headers_t * headers, header_t * hlist, int maxlen);
int headers_add(headers_t * headers, const char * name, const char * value);
int headers_set(headers_t * headers, const char * name, const char * value);
int headers_get(header_t * headers, const char * name, char * value);
int headers_count(header_t * headers);

#endif
