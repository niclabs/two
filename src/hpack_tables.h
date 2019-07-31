#ifndef TWO_HPACK_TABLES_H
#define TWO_HPACK_TABLES_H
#include "logging.h"
#include <stdint.h>
#define STATIC_TABLE_SIZE 61

//typedef for HeaderPair
typedef struct hpack_header_pair {
    char *name;
    char *value;
} header_pair_t;

typedef struct {
    const char *const name_table[STATIC_TABLE_SIZE];
    const char *const value_table[STATIC_TABLE_SIZE];
} hpack_static_table_t;

//typedefs for dinamic
//TODO check size of struct
typedef struct hpack_dynamic_table {
    uint32_t max_size;
    uint32_t first;
    uint32_t next;
    uint32_t table_length;
    header_pair_t *table;
} hpack_dynamic_table_t;

int8_t hpack_tables_static_find_name_and_value(uint8_t index, char *name, char *value);
int8_t hpack_tables_static_find_name(uint8_t index, char *name);
uint32_t hpack_tables_header_pair_size(header_pair_t header_pair);
int hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
int hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);
uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size);
int hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t* table);
int hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);

#endif //TWO_HPACK_TABLES_H
