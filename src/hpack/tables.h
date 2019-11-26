#ifndef TWO_HPACK_TABLES_H
#define TWO_HPACK_TABLES_H
#include <stdint.h>             /* for int8_t, uint32_t */
#include "hpack/structs.h"

#define STATIC_TABLE_SIZE (61)


typedef struct {
    const char *const name_table[STATIC_TABLE_SIZE];
    const char *const value_table[STATIC_TABLE_SIZE];
} hpack_static_table_t;



int8_t hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
int8_t hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name);
#if HPACK_INCLUDE_DYNAMIC_TABLE
uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size);
void hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size);
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_size, uint32_t new_max_size);
#endif  //HPACK_INCLUDE_DYNAMIC_TABLE

#endif  //TWO_HPACK_TABLES_H
