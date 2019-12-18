#ifndef TWO_HPACK_TABLES_H
#define TWO_HPACK_TABLES_H
#include <stdint.h>             /* for int8_t, uint32_t */
#include "hpack/structs.h"

#define STATIC_TABLE_SIZE (61)
#define SEPARATOR "\0"

#define CREATE_STATIC_TABLE(n0, n1, n2, n3, n4,  \
                            n5, n6, n7, n8, n9,  \
                            n10,n11,n12,n13,n14, \
                            n15,n16,n17,n18,n19, \
                            n20,n21,n22,n23,n24, \
                            n25,n26,n27,n28,n29, \
                            n30,n31,n32,n33,n34, \
                            n35,n36,n37,n38,n39, \
                            n40,n41,n42,n43,n44, \
                            n45,n46,n47,n48,n49, \
                            n50,n51,n52,n53,n54, \
                            n55,n56,n57,n58,n59, \
                            n60)                 \
                            n0 SEPARATOR n1 SEPARATOR n2 SEPARATOR n3 SEPARATOR n4 SEPARATOR      \
                            n5 SEPARATOR n6 SEPARATOR n7 SEPARATOR n8 SEPARATOR n9 SEPARATOR      \
                            n10 SEPARATOR n11 SEPARATOR n12 SEPARATOR n13 SEPARATOR n14 SEPARATOR \
                            n15 SEPARATOR n16 SEPARATOR n17 SEPARATOR n18 SEPARATOR n19 SEPARATOR \
                            n20 SEPARATOR n21 SEPARATOR n22 SEPARATOR n23 SEPARATOR n24 SEPARATOR \
                            n25 SEPARATOR n26 SEPARATOR n27 SEPARATOR n28 SEPARATOR n29 SEPARATOR \
                            n30 SEPARATOR n31 SEPARATOR n32 SEPARATOR n33 SEPARATOR n34 SEPARATOR \
                            n35 SEPARATOR n36 SEPARATOR n37 SEPARATOR n38 SEPARATOR n39 SEPARATOR \
                            n40 SEPARATOR n41 SEPARATOR n42 SEPARATOR n43 SEPARATOR n44 SEPARATOR \
                            n45 SEPARATOR n46 SEPARATOR n47 SEPARATOR n48 SEPARATOR n49 SEPARATOR \
                            n50 SEPARATOR n51 SEPARATOR n52 SEPARATOR n53 SEPARATOR n54 SEPARATOR \
                            n55 SEPARATOR n56 SEPARATOR n57 SEPARATOR n58 SEPARATOR n59 SEPARATOR \
                            n60 SEPARATOR                                                         \

#define NAME_TABLE_LITERAL CREATE_STATIC_TABLE(":authority",                        \
                                                ":method",                          \
                                                ":method",                          \
                                                ":path",                            \
                                                ":path",                            \
                                                ":scheme",                          \
                                                ":scheme",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                ":status",                          \
                                                "accept-charset",                   \
                                                "accept-encoding",                  \
                                                "accept-language",                  \
                                                "accept-ranges",                    \
                                                "accept",                           \
                                                "access-control-allow-origin",      \
                                                "age",                              \
                                                "allow",                            \
                                                "authorization",                    \
                                                "cache-control",                    \
                                                "content-disposition",              \
                                                "content-encoding",                 \
                                                "content-language",                 \
                                                "content-length",                   \
                                                "content-location",                 \
                                                "content-range",                    \
                                                "content-type",                     \
                                                "cookie",                           \
                                                "date",                             \
                                                "etag",                             \
                                                "expect",                           \
                                                "expires",                          \
                                                "from",                             \
                                                "host",                             \
                                                "if-match",                         \
                                                "if-modified-since",                \
                                                "if-none-match",                    \
                                                "if-range",                         \
                                                "if-unmodified-since",              \
                                                "last-modified",                    \
                                                "link",                             \
                                                "location",                         \
                                                "max-forwards",                     \
                                                "proxy-authenticate",               \
                                                "proxy-authorization",              \
                                                "range",                            \
                                                "referer",                          \
                                                "refresh",                          \
                                                "retry-after",                      \
                                                "server",                           \
                                                "set-cookie",                       \
                                                "strict-transport-security",        \
                                                "transfer-encoding",                \
                                                "user-agent",                       \
                                                "vary",                             \
                                                "via",                              \
                                                "www-authenticate")                 \

#define VALUE_TABLE_LITERAL CREATE_STATIC_TABLE("",                 \
                                                "GET",              \
                                                "POST",             \
                                                "/",                \
                                                "/index.html",      \
                                                "http",             \
                                                "https",            \
                                                "200",              \
                                                "204",              \
                                                "206",              \
                                                "304",              \
                                                "400",              \
                                                "404",              \
                                                "500",              \
                                                "",                 \
                                                "gzip, deflate",    \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "",                 \
                                                "")                 \

#define STATIC_TABLE_NAME_SIZE sizeof(NAME_TABLE_LITERAL)
#define STATIC_TABLE_VALUE_SIZE sizeof(VALUE_TABLE_LITERAL)

typedef struct {
    char name_table[STATIC_TABLE_NAME_SIZE];
    char value_table[STATIC_TABLE_VALUE_SIZE];
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
