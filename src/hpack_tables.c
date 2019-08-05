#include "hpack_tables.h"
#include "headers.h"

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
int8_t hpack_tables_static_find_entry_name_and_value(uint8_t index, char *name, char *value)
{
    index--; //because static table begins at index 1
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        return -1;
    }
    const char *table_name = hpack_static_table.name_table[index];
    const char *table_value = hpack_static_table.value_table[index];
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
int8_t hpack_tables_static_find_entry_name(uint8_t index, char *name)
{
    index--;
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        return -1;
    }
    const char *table_name = hpack_static_table.name_table[index];
    strncpy(name, table_name, strlen(table_name));
    return 0;
}

/*
 * Function: hpack_tables_dynamic_table_length
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 * Output:
 *      returns the actual table length which is equal to the number of entries in the table_length
 */
uint32_t hpack_tables_dynamic_table_length(hpack_dynamic_table_t *dynamic_table)
{
    uint32_t table_length_used = dynamic_table->first < dynamic_table->next ?
                                 (dynamic_table->next - dynamic_table->first) % dynamic_table->table_length :
                                 (dynamic_table->table_length - dynamic_table->first + dynamic_table->next) % dynamic_table->table_length;

    return table_length_used;
}

/*
 * Function: hpack_tables_dynamic_find_entry_name_and_value
 * Finds entry in dynamic table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 * Output:
 *      0 if success, -1 in case of Error
 */
int8_t hpack_tables_dynamic_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value)
{
    if (hpack_tables_dynamic_table_length(dynamic_table) < index - 61) { //CASE entry doesnt exist
        return -1;
    }

    uint32_t table_index = (dynamic_table->next + dynamic_table->table_length - (index - 61)) % dynamic_table->table_length;

    header_pair_t result = dynamic_table->table[table_index];
    strncpy(name, result.name, strlen(result.name));
    strncpy(value, result.value, strlen(result.value));
    return 0;
}

/*
 * Function: hpack_tables_dynamic_find_entry_name
 * Finds entry in dynamic table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: Buffer to store name of the header
 * Output:
 *      0 if success, -1 in case of Error
 */
int8_t hpack_tables_dynamic_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    uint32_t table_index = (dynamic_table->next + dynamic_table->table_length - (index - 61)) % dynamic_table->table_length;

    if (hpack_tables_dynamic_table_length(dynamic_table) < index - 61) { //CASE entry doesnt exist
        return -1;
    }
    header_pair_t result = dynamic_table->table[table_index];
    strncpy(name, result.name, strlen(result.name));
    return 0;
}

/*
 * Function: hpack_tables_header_pair_size
 * Input:
 *      -> header_pair:
 * Output:
 *      return the size of a header HeaderPair, the size is
 *      the sum of octets used for its encoding and 32
 */
uint32_t hpack_tables_header_pair_size(header_pair_t header_pair)
{
    return (uint32_t)(strlen(header_pair.name) + strlen(header_pair.value) + 32);
}


/*
 * Function: hpack_tables_dynamic_table_size
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 * Output:
 *      returns the size of the table, this is the sum of each header pair's size
 */
uint32_t hpack_tables_dynamic_table_size(hpack_dynamic_table_t *dynamic_table)
{
    uint32_t total_size = 0;
    uint32_t table_length = hpack_tables_dynamic_table_length(dynamic_table);

    for (uint32_t i = 0; i < table_length; i++) {
        total_size += hpack_tables_header_pair_size(dynamic_table->table[(i + dynamic_table->first) % table_length]);
    }
    return total_size;
}

/* TODO
 * Function: hpack_tables_dynamic_table_resize
 * Makes an update of the size of the dynamic table_length
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> new_max_size: new virtual max size of the table, setted in the header of resize
 *      -> dynamic_table_max_size: Max size in bytes of dynamic table setted in SETTINGS
 * Output:
 *      return 0 if the update is succesful, or -1 otherwise
 */
int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t new_max_size, uint32_t dynamic_table_max_size)
{
    if (new_max_size > dynamic_table_max_size) {
        ERROR("Resize operation exceeds the maximum size set by the protocol ");
        return -1;
    }
    //TODO CHECK SIZE
    uint32_t new_table_length = (uint32_t)(new_max_size / sizeof(header_pair_t)) + 1;
    header_pair_t new_table[new_table_length];

    uint32_t new_first = 0;
    uint32_t new_next = 0;
    uint32_t new_size = 0;

    uint32_t i = dynamic_table->first;
    uint32_t j = 0;

    uint32_t table_length_used = hpack_tables_dynamic_table_length(dynamic_table);

    while (j < table_length_used && (new_size + hpack_tables_header_pair_size(dynamic_table->table[i])) <= new_max_size) {
        new_table[j] = dynamic_table->table[i];
        i = (i + 1) % dynamic_table->table_length;
        j++;
        new_next = (new_next + 1) % new_table_length;
        new_size += hpack_tables_header_pair_size(dynamic_table->table[i]);
    }

    //clean last table_length
    memset(&dynamic_table->table, 0, sizeof(dynamic_table->table));

    dynamic_table->max_size = new_max_size;
    dynamic_table->table_length = new_table_length;
    dynamic_table->next = new_next;
    dynamic_table->first = new_first;
    dynamic_table->table = new_table;


    return 0;

}



/*
 * Function: dynamic_table_add_entry
 * Add an header pair entry in the table
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> *name: New entry name added
 *      -> *value: New entry value added
 * Output:
 *      0 if success, -1 otherwise
 */
//header pair is a name string and a value string
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    uint32_t entry_size = (uint32_t)(strlen(name) + strlen(value) + 32);

    if (entry_size > dynamic_table->max_size) {
        ERROR("New entry size exceeds the size of table ");
        return -1; //entry's size exceeds the size of table
    }

    while (entry_size + hpack_tables_dynamic_table_size(dynamic_table) > dynamic_table->max_size) {
        dynamic_table->first = (dynamic_table->first + 1) % dynamic_table->table_length;
    }

    dynamic_table->table[dynamic_table->next].name = name;
    dynamic_table->table[dynamic_table->next].value = value;
    dynamic_table->next = (dynamic_table->next + 1) % dynamic_table->table_length;
    return 0;
}

/*
 * Function: find_entry_name_and_value
 * finds an entry (pair name-value) in either the static or dynamic table_length
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 *      -> *value: buffer where the value of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int8_t hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name, char *value)
{

    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }

        int8_t rc = hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, index, name, value);
        if (rc < 0) {
            ERROR("The entry doesn't exist in dynamic table");
            return -1;
        }
    }
    else {
        int8_t rc = hpack_tables_static_find_entry_name_and_value(index, name, value);
        if (rc < 0) {
            ERROR("The index was greater than the size of the static table");
            return -1;
        }
    }
    return 0;
}

/*
 * Function: find_entry_name
 * finds an entry name in either the static or dynamic table_length
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: buffer where the name of the entry will be stored
 * Output:
 *      0 if success, -1 in case of Error
 */
int8_t hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }
        int8_t rc = hpack_tables_dynamic_find_entry_name(dynamic_table, index, name);
        if (rc < 0) {
            ERROR("The entry doesn't exist in dynamic table");
            return -1;
        }
    }
    else {
        int8_t rc = hpack_tables_static_find_entry_name(index, name);
        if (rc < 0) {
            ERROR("The index was greater than the size of the static table");
            return -1;
        }
    }
    return 0;
}

/*
 *  Function: hpack_tables_find_index
 *  Given a buffer containing a name and another buffer containing the value of a header
 *  Searches both static and Dynamic tables
 *  Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> *name: Buffer containing the name of a header to search
 *      -> *value: Buffer containing the value of a header to search
 *  Output:
 *      Returns the index in the static or dynamic table containing both name and value if successful,
 *      otherwise it returns -1.
 */
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    //TODO Check if name and value has to match or only name!!

    //Search first in static table
    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        const char *table_name = hpack_static_table.name_table[i];
        const char *table_value = hpack_static_table.value_table[i];
        if ((strncmp(table_name, name, strlen(name)) == 0) &&
            (strncmp(table_value, value, strlen(value)) == 0)) {
            return i + 1;
        }
    }
    //Then search in dynamic table
    char tmp_name[MAX_HEADER_NAME_LEN];
    char tmp_value[MAX_HEADER_VALUE_LEN];
    for (uint8_t i=0; i < hpack_tables_dynamic_table_length(dynamic_table); i++){
        hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, i+HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name, tmp_value);
        if ((strncmp(tmp_name, name, strlen(name)) == 0) &&
            (strncmp(tmp_value, value, strlen(value)) == 0)) {
            return i + HPACK_TABLES_FIRST_INDEX_DYNAMIC;
        }
    }

    return -1;
}

/*
 *  Function: hpack_tables_find_index_name
 *  Given a buffer containing a name a header
 *  Searches both static and Dynamic tables
 *  Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> *name: Buffer containing the name of a header to search
 *  Output:
 *      Returns the index in the static or dynamic table containing name if successful,
 *      otherwise it returns -1.
 */
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name)
{
    //TODO Check if name and value has to match or only name!!

    //Search first in static table
    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        const char *table_name = hpack_static_table.name_table[i];
        if (strncmp(table_name, name, strlen(name)) == 0) {
            return i + 1;
        }
    }
    //Then search in dynamic table
    char tmp_name[MAX_HEADER_NAME_LEN];
    for (uint8_t i=0; i < hpack_tables_dynamic_table_length(dynamic_table); i++){
        hpack_tables_dynamic_find_entry_name(dynamic_table, i+HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name);
        if (strncmp(tmp_name, name, strlen(name)) == 0) {
            return i + HPACK_TABLES_FIRST_INDEX_DYNAMIC;
        }
    }

    return -1;
}

uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size)
{
    return (uint32_t)((dynamic_table_size / sizeof(header_pair_t)) + 1);
}

/*
 * Function: hpack_init_dynamic_table
 * Initialize dynamic_table for protocol uses, but it requires a previous header_pair_t table initialization
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> dynamic_table_max_size: Max size in bytes of new dynamic_table, it is set in HTTP SETTING-
 * Output:
 *     0 if success
 */
int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, header_pair_t *table)
{
    memset(dynamic_table, 0, sizeof(hpack_dynamic_table_t));
    dynamic_table->max_size = dynamic_table_max_size;
    dynamic_table->table_length = hpack_tables_get_table_length(dynamic_table_max_size);
    dynamic_table->first = 0;
    dynamic_table->next = 0;
    dynamic_table->table = table;
    return 0;
}
