#include "hpack_tables.h"

const uint32_t HPACK_TABLES_FIRST_INDEX_DYNAMIC = 62; // Changed type to remove warnings

//HeaderPairs in static table

const char name_0[] = ":authority";
const char name_1[] = ":method";
const char name_2[] = ":method";
const char name_3[] = ":path";
const char name_4[] = ":path";
const char name_5[] = ":scheme";

const char name_6[] = ":scheme";
const char name_7[] = ":status";
const char name_8[] = ":status";
const char name_9[] = ":status";
const char name_10[] = ":status";

const char name_11[] = ":status";
const char name_12[] = ":status";
const char name_13[] = ":status";
const char name_14[] = "accept-charset";
const char name_15[] = "accept-encoding";

const char name_16[] = "accept-language";
const char name_17[] = "accept-ranges";
const char name_18[] = "accept";
const char name_19[] = "access-control-allow-origin";
const char name_20[] = "age";

const char name_21[] = "allow";
const char name_22[] = "authorization";
const char name_23[] = "cache-control";
const char name_24[] = "content-disposition";
const char name_25[] = "content-encoding";

const char name_26[] = "content-language";
const char name_27[] = "content-length";
const char name_28[] = "content-location";
const char name_29[] = "content-range";
const char name_30[] = "content-type";

const char name_31[] = "cookie";
const char name_32[] = "date";
const char name_33[] = "etag";
const char name_34[] = "expect";
const char name_35[] = "expires";

const char name_36[] = "from";
const char name_37[] = "host";
const char name_38[] = "if-match";
const char name_39[] = "if-modified-since";
const char name_40[] = "if-none-match";

const char name_41[] = "if-range";
const char name_42[] = "if-unmodified-since";
const char name_43[] = "last-modified";
const char name_44[] = "link";
const char name_45[] = "location";

const char name_46[] = "max-forwards";
const char name_47[] = "proxy-authenticate";
const char name_48[] = "proxy-authorization";
const char name_49[] = "range";
const char name_50[] = "referer";

const char name_51[] = "refresh";
const char name_52[] = "retry-after";
const char name_53[] = "server";
const char name_54[] = "set-cookie";
const char name_55[] = "strict-transport-security";

const char name_56[] = "transfer-encoding";
const char name_57[] = "user-agent";
const char name_58[] = "vary";
const char name_59[] = "via";
const char name_60[] = "www-authenticate";
//static table header values

const char value_0[] = "";
const char value_1[] = "GET";
const char value_2[] = "POST";
const char value_3[] = "/";
const char value_4[] = "/index.html";
const char value_5[] = "http";

const char value_6[] = "https";
const char value_7[] = "200";
const char value_8[] = "204";
const char value_9[] = "206";
const char value_10[] = "304";

const char value_11[] = "400";
const char value_12[] = "404";
const char value_13[] = "500";
const char value_14[] = "";
const char value_15[] = "gzip, deflate";

const char value_16[] = "";
const char value_17[] = "";
const char value_18[] = "";
const char value_19[] = "";
const char value_20[] = "";

const char value_21[] = "";
const char value_22[] = "";
const char value_23[] = "";
const char value_24[] = "";
const char value_25[] = "";

const char value_26[] = "";
const char value_27[] = "";
const char value_28[] = "";
const char value_29[] = "";
const char value_30[] = "";

const char value_31[] = "";
const char value_32[] = "";
const char value_33[] = "";
const char value_34[] = "";
const char value_35[] = "";

const char value_36[] = "";
const char value_37[] = "";
const char value_38[] = "";
const char value_39[] = "";
const char value_40[] = "";

const char value_41[] = "";
const char value_42[] = "";
const char value_43[] = "";
const char value_44[] = "";
const char value_45[] = "";

const char value_46[] = "";
const char value_47[] = "";
const char value_48[] = "";
const char value_49[] = "";
const char value_50[] = "";

const char value_51[] = "";
const char value_52[] = "";
const char value_53[] = "";
const char value_54[] = "";
const char value_55[] = "";

const char value_56[] = "";
const char value_57[] = "";
const char value_58[] = "";
const char value_59[] = "";
const char value_60[] = "";

const hpack_static_table_t hpack_static_table = {
    .name_table = {
        name_0,
        name_1,
        name_2,
        name_3,
        name_4,
        name_5,
        name_6,
        name_7,
        name_8,
        name_9,
        name_10,
        name_11,
        name_12,
        name_13,
        name_14,
        name_15,
        name_16,
        name_17,
        name_18,
        name_19,
        name_20,
        name_21,
        name_22,
        name_23,
        name_24,
        name_25,
        name_26,
        name_27,
        name_28,
        name_29,
        name_30,
        name_31,
        name_32,
        name_33,
        name_34,
        name_35,
        name_36,
        name_37,
        name_38,
        name_39,
        name_40,
        name_41,
        name_42,
        name_43,
        name_44,
        name_45,
        name_46,
        name_47,
        name_48,
        name_49,
        name_50,
        name_51,
        name_52,
        name_53,
        name_54,
        name_55,
        name_56,
        name_57,
        name_58,
        name_59,
        name_60
    },
    .value_table = {
        value_0,
        value_1,
        value_2,
        value_3,
        value_4,
        value_5,
        value_6,
        value_7,
        value_8,
        value_9,
        value_10,
        value_11,
        value_12,
        value_13,
        value_14,
        value_15,
        value_16,
        value_17,
        value_18,
        value_19,
        value_20,
        value_21,
        value_22,
        value_23,
        value_24,
        value_25,
        value_26,
        value_27,
        value_28,
        value_29,
        value_30,
        value_31,
        value_32,
        value_33,
        value_34,
        value_35,
        value_36,
        value_37,
        value_38,
        value_39,
        value_40,
        value_41,
        value_42,
        value_43,
        value_44,
        value_45,
        value_46,
        value_47,
        value_48,
        value_49,
        value_50,
        value_51,
        value_52,
        value_53,
        value_54,
        value_55,
        value_56,
        value_57,
        value_58,
        value_59,
        value_60
    }
};

/*
 * Function: hpack_tables_static_find_name_and_value
 * finds the entry in the given index and copies the result to name and value
 * Input:
 *      -> index: Index of the static table to retrieve
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 */
int8_t hpack_tables_static_find_name_and_value(uint8_t index, char *name, char *value){
    index--; //because static table begins at index 1
    if(index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC){
        return -1;
    }
    const char* table_name = hpack_static_table.name_table[index];
    const char* table_value = hpack_static_table.value_table[index];
    strncpy(name, table_name, strlen(table_name));
    strncpy(value, table_value, strlen(table_value));
    return 0;
}

/*
 * Function: hpack_tables_static_find_name_and_value
 * finds the entry in the given index and copies the result to name and value
 * Input:
 *      -> index: Index of the static table to retrieve
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 */
int8_t hpack_tables_static_find_name(uint8_t index, char *name){
    index--;
    if(index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC){
        return -1;
    }
    const char* table_name = hpack_static_table.name_table[index];
    strncpy(name, table_name, strlen(table_name));
    return 0;
}