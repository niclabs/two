#ifndef TWO_HPACK_TABLES_H
#define TWO_HPACK_TABLES_H
#include <stdint.h>             /* for int8_t, uint32_t */

#define STATIC_TABLE_SIZE (61)
#ifdef HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE)
#else
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (4092)
#endif

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



int8_t hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
int8_t hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);
uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size);
int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t* table);
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name);

#endif //TWO_HPACK_TABLES_H
