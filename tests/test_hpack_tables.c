#include "unit.h"
#include "fff.h"
#include "hpack_tables.h"

extern int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
extern const hpack_static_table_t hpack_static_table;
extern int8_t hpack_tables_static_find_entry_name_and_value(uint8_t index, char *name, char *value);
extern int8_t hpack_tables_static_find_entry_name(uint8_t index, char *name);
extern int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
extern int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t* table);
extern int8_t hpack_tables_dynamic_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value);
extern int8_t hpack_tables_dynamic_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name);

DEFINE_FFF_GLOBALS;

void setUp(void)
{
    /* Register resets */
    /*FFF_FAKES_LIST(RESET_FAKE);*/

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_hpack_tables_find_index(void)
{
    int expected_index[] = { 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 16 };
    char *names[] = { ":method", ":method", ":path", ":path", ":scheme", ":scheme", ":status", ":status", ":status", ":status", ":status", ":status", ":status", "accept-encoding" };
    char *values[] = { "GET", "POST", "/", "/index.html", "http", "https", "200", "204", "206", "304", "400", "404", "500", "gzip, deflate" };

    for(int i = 0; i < 14; i++){
        int actual_index = hpack_tables_find_index(NULL, names[i],values[i]);
        TEST_ASSERT_EQUAL(expected_index[i],actual_index);
    }
}

void test_hpack_tables_static_find_entry_name_and_value(void)
{
   char *expected_name[] = {":authority",":method",":method","accept-encoding",":status","content-range","if-unmodified-since"};
   char *expected_value[] = {"","POST","GET","gzip, deflate","404","",""};
   uint32_t example_index[] = {1,3,2,16,13,30,43};

   char name[30];
   char value[20];

   for(int i=0; i<7; i++){
     memset(name, 0, sizeof(name));
     memset(value, 0, sizeof(value));
     hpack_tables_static_find_entry_name_and_value(example_index[i], name, value);
     TEST_ASSERT_EQUAL_STRING(name, expected_name[i]);
     TEST_ASSERT_EQUAL_STRING(value, expected_value[i]);
   }
}
void test_hpack_tables_static_find_entry_name(void)
{
  char *expected_name[] = {":path","age","accept","allow","cookie"};
  uint32_t example_index[] = {5,21,19,22,32};

  char name[30];

  for(int i=0; i<5; i++){
    memset(name, 0, sizeof(name));
    hpack_tables_static_find_entry_name(example_index[i], name);
    TEST_ASSERT_EQUAL_STRING(name, expected_name[i]);
  }
}
int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_hpack_tables_find_index);
    UNIT_TEST(test_hpack_tables_static_find_entry_name_and_value);
    UNIT_TEST(test_hpack_tables_static_find_entry_name);
    return UNITY_END();
}
