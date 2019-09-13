#ifndef TWO_HPACK_TABLES_H
#define TWO_HPACK_TABLES_H
#include <stdint.h>             /* for int8_t, uint32_t */

//TODO add macros to conf file
#define HPACK_CONF_INCLUDE_DYNAMIC_TABLE (1)

#define STATIC_TABLE_SIZE (61)
#ifdef HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (HPACK_CONF_MAX_DYNAMIC_TABLE_SIZE)
#else
#define HPACK_MAX_DYNAMIC_TABLE_SIZE (4096)
#endif
#ifdef HPACK_CONF_INCLUDE_DYNAMIC_TABLE
#define HPACK_INCLUDE_DYNAMIC_TABLE (HPACK_CONF_INCLUDE_DYNAMIC_TABLE)
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
//size: 10 bytes in params + size of buffer
typedef struct hpack_dynamic_table {
    uint16_t max_size;
    uint16_t first;
    uint16_t next;
    uint16_t actual_size;
    uint16_t n_entries;
    char buffer[HPACK_MAX_DYNAMIC_TABLE_SIZE];
} hpack_dynamic_table_t;



int8_t hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
int8_t hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value, char *tmp_name, char *tmp_value);
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name, char *tmp_name);
#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size);
int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size);
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t settings_max_size, uint32_t new_max_size);
#endif  //HPACK_INCLUDE_DYNAMIC_TABLE

#endif  //TWO_HPACK_TABLES_H
