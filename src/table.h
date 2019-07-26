#ifndef TABLE_H
#define TABLE_H

#include <stdint.h>

#define HTTP2_MAX_HBF_BUFFER 128
#define HTTP_MAX_DATA_SIZE 128

typedef struct HEADERS_DATA_LISTS_S {
    uint32_t data_in_size;
    uint8_t data_in[HTTP_MAX_DATA_SIZE];
    uint32_t data_out_size;
    uint8_t data_out[HTTP_MAX_DATA_SIZE];
    uint32_t data_out_sent;
    uint32_t data_in_received;
} headers_data_lists_t;


#endif /* TABLE_H */
