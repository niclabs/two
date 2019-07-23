#ifndef HEADERS_H
#define HEADERS_H

#include <stdio.h>

#ifndef CONF_MAX_HEADER_NAME_LEN
#define MAX_HEADER_NAME_LEN (16)
#else
#define MAX_HEADER_NAME_LEN (CONF_MAX_HEADER_NAME_LEN)
#endif

#ifndef CONF_MAX_HEADER_VALUE_LEN
#define MAX_HEADER_VALUE_LEN (32)
#else
#define MAX_HEADER_VALUE_LEN (CONF_MAX_HEADER_VALUE_LEN)
#endif


typedef struct {
    char name[MAX_HEADER_NAME_LEN + 1];
    char value[MAX_HEADER_VALUE_LEN + 1];
} header_t;

typedef struct {
    header_t * headers;
    int count;
    int maxlen;
} headers_t;


int headers_init(headers_t * headers, header_t * hlist, int maxlen);
int headers_add(headers_t * headers, const char * name, const char * value);
int headers_set(headers_t * headers, const char * name, const char * value);
int headers_get(headers_t * headers, const char * name, char * value);
int headers_get_len(headers_t * headers, const char * name, char * value, size_t len);
int headers_count(headers_t * headers);

#endif
