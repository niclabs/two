#include "logging.h"
#include <stdint.h>
#define STATIC_TABLE_SIZE 61

typedef struct {
    const char *const name_table[STATIC_TABLE_SIZE];
    const char *const value_table[STATIC_TABLE_SIZE];
} hpack_static_table_t;

int8_t hpack_tables_static_find_name_and_value(uint8_t index, char *name, char *value);
int8_t hpack_tables_static_find_name(uint8_t index, char *name);