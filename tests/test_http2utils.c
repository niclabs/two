#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2utils.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, http_read, hstates_t*, uint8_t*, int);


#define FFF_FAKES_LIST(FAKE)        \
    FAKE(http_read)

void setUp(void) {
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_get_setting_value(void){
    uint32_t settings[6];
    settings[0] = 0;
    settings[1] = 123123;
    settings[2] = 1234566;
    settings[3] = 16;
    settings[4] = 123;
    settings[5] = 987432958;

    uint32_t value_received;
    value_received = get_setting_value(settings, HEADER_TABLE_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[0]);
    value_received = get_setting_value(settings, ENABLE_PUSH);
    TEST_ASSERT_EQUAL(value_received, settings[1]);
    value_received = get_setting_value(settings, MAX_CONCURRENT_STREAMS);
    TEST_ASSERT_EQUAL(value_received, settings[2]);
    value_received = get_setting_value(settings, INITIAL_WINDOW_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[3]);
    value_received = get_setting_value(settings, MAX_FRAME_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[4]);
    value_received = get_setting_value(settings, MAX_HEADER_LIST_SIZE);
    TEST_ASSERT_EQUAL(value_received, settings[5]);
}



void test_get_header_list_size(void){
    table_pair_t table_pair[2];
    strcpy(table_pair[0].name, "name1");//5
    strcpy(table_pair[0].value, "value1");//6
    //table_pair[0].value = "value1";
    //table_pair[1].name = "anothername";
    strcpy(table_pair[1].name, "anothername");//11
    strcpy(table_pair[1].value, "anothervalue");//12
    //table_pair[1].value = "anothervalue";

    uint32_t expected_size =(uint32_t) strlen(table_pair[0].name) + strlen(table_pair[0].value) + 64 + strlen(table_pair[1].name) + strlen(table_pair[1].value) + 64;//adds 32 for the overhead of every header name and value (check RFC)

    uint32_t result = get_header_list_size(table_pair,2);
    TEST_ASSERT_EQUAL(result, expected_size);
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_get_setting_value);
    UNIT_TEST(test_get_header_list_size);

    return UNIT_TESTS_END();
}
