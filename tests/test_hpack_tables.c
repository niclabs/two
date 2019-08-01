#include "unit.h"
#include "fff.h"
#include "hpack_tables.h"

extern int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value);
extern const hpack_static_table_t hpack_static_table;

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
int main(void)
{
    UNITY_BEGIN();
    UNIT_TEST(test_hpack_tables_find_index);
    return UNITY_END();
}
