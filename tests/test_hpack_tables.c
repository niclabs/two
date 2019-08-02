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
     TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
     TEST_ASSERT_EQUAL_STRING(expected_value[i], value);
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
    TEST_ASSERT_EQUAL_STRING(expected_name[i], name);
  }
}

void test_hpack_tables_dynamic_add_find_entry(void)
{
  uint32_t dynamic_table_max_size = 500;
  header_pair_t table[hpack_tables_get_table_length(dynamic_table_max_size)];
  hpack_dynamic_table_t dynamic_table;

  hpack_tables_init_dynamic_table(&dynamic_table, dynamic_table_max_size, table);

  char *new_names[] = {"hola","sol3","bien1"};
  char *new_values[] = {"chao","luna4","mal2"};

  char name[30];
  char value[20];

  TEST_ASSERT_EQUAL(0,dynamic_table.first);
  TEST_ASSERT_EQUAL(0,dynamic_table.next);

  //ITERATION 1, add first entry
  hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[0], new_values[0]);
  TEST_ASSERT_EQUAL(0, dynamic_table.first);
  TEST_ASSERT_EQUAL(1, dynamic_table.next);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[0], name);
  TEST_ASSERT_EQUAL_STRING(new_values[0], value);

  //ITERATION 2, add second entry
  hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[1], new_values[1]);
  TEST_ASSERT_EQUAL(0, dynamic_table.first);
  TEST_ASSERT_EQUAL(2, dynamic_table.next);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[1], name);
  TEST_ASSERT_EQUAL_STRING(new_values[1], value);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value); //63 -> second value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[0], name);
  TEST_ASSERT_EQUAL_STRING(new_values[0], value);

  //ITERATION 3, add third entry
  hpack_tables_dynamic_table_add_entry(&dynamic_table, new_names[2], new_values[2]);
  TEST_ASSERT_EQUAL(0, dynamic_table.first);
  TEST_ASSERT_EQUAL(3, dynamic_table.next);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 62, name, value); //62 -> first value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[2], name);
  TEST_ASSERT_EQUAL_STRING(new_values[2], value);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 63, name, value); //63 -> second value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[1], name);
  TEST_ASSERT_EQUAL_STRING(new_values[1], value);

  memset(name, 0, sizeof(name));
  memset(value, 0, sizeof(value));
  hpack_tables_dynamic_find_entry_name_and_value(&dynamic_table, 64, name, value); //64 -> third value of dynamic table
  TEST_ASSERT_EQUAL_STRING(new_names[0], name);
  TEST_ASSERT_EQUAL_STRING(new_values[0], value);
}

int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_hpack_tables_find_index);
    UNIT_TEST(test_hpack_tables_static_find_entry_name_and_value);
    UNIT_TEST(test_hpack_tables_static_find_entry_name);
    UNIT_TEST(test_hpack_tables_dynamic_add_find_entry);
    return UNITY_END();
}
