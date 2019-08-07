#include "hpack_tables.h"
#include "headers.h"                                    /* for MAX_HEADER_NAME_LEN, MAX_HEADER_VALUE_LEN  */
#include "logging.h"                                    /* for ERROR*/
#include <string.h>                                     /* for strlen, strncmp, memset*/

const uint32_t HPACK_TABLES_FIRST_INDEX_DYNAMIC = 62;   // Changed type to remove warnings

//HeaderPairs in static table

const hpack_static_table_t hpack_static_table = {
    .name_table = {
        ":authority",
        ":method",
        ":method",
        ":path",
        ":path",
        ":scheme",
        ":scheme",
        ":status",
        ":status",
        ":status",
        ":status",
        ":status",
        ":status",
        ":status",
        "accept-charset",
        "accept-encoding",
        "accept-language",
        "accept-ranges",
        "accept",
        "access-control-allow-origin",
        "age",
        "allow",
        "authorization",
        "cache-control",
        "content-disposition",
        "content-encoding",
        "content-language",
        "content-length",
        "content-location",
        "content-range",
        "content-type",
        "cookie",
        "date",
        "etag",
        "expect",
        "expires",
        "from",
        "host",
        "if-match",
        "if-modified-since",
        "if-none-match",
        "if-range",
        "if-unmodified-since",
        "last-modified",
        "link",
        "location",
        "max-forwards",
        "proxy-authenticate",
        "proxy-authorization",
        "range",
        "referer",
        "refresh",
        "retry-after",
        "server",
        "set-cookie",
        "strict-transport-security",
        "transfer-encoding",
        "user-agent",
        "vary",
        "via",
        "www-authenticate",
    },
    .value_table = {
            "",
            "GET",
            "POST",
            "/",
            "/index.html",
            "http",
            "https",
            "200",
            "204",
            "206",
            "304",
            "400",
            "404",
            "500",
            "",
            "gzip, deflate",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
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

    uint32_t new_table_length = (uint32_t)((new_max_size / 32) + 1);

    //TODO check case new max size different but length same
    //Case same size as before
    if(new_max_size == dynamic_table->max_size){
        return 0;
    }
    //Case table grows
    if(new_table_length > dynamic_table->table_length){
      //if next is after first
      if(dynamic_table->first < dynamic_table->next){
        dynamic_table->max_size = new_max_size;
        dynamic_table->table_length = new_table_length;
      }
      //if next is before first
      else{
        if(dynamic_table->next < new_table_length - dynamic_table->table_length){
          for(uint32_t i=0; i < dynamic_table->next; i++){
            dynamic_table->table[dynamic_table->table_length+i].name = dynamic_table->table[i].name;
            dynamic_table->table[dynamic_table->table_length+i].value = dynamic_table->table[i].value;
          }
          dynamic_table->next = dynamic_table->table_length + dynamic_table->next;
        }
        else{
          for(uint32_t i=0; i < new_table_length - dynamic_table->table_length; i++){
            dynamic_table->table[dynamic_table->table_length+i].name = dynamic_table->table[i].name;
            dynamic_table->table[dynamic_table->table_length+i].value = dynamic_table->table[i].value;
          }
          for(uint32_t i=0; i < dynamic_table->next - (new_table_length - dynamic_table->table_length); i++){
            dynamic_table->table[i].name = dynamic_table->table[new_table_length-dynamic_table->table_length + i].name;
            dynamic_table->table[i].value = dynamic_table->table[new_table_length-dynamic_table->table_length + i].value;
          }
          dynamic_table->next = new_table_length - dynamic_table->table_length;
        }
      }
      return 0;
    }
    //Case table shrinks
    //else{
      //TODO
    //}
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
    for (uint8_t i = 0; i < hpack_tables_dynamic_table_length(dynamic_table); i++) {
        hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, i + HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name, tmp_value);
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

    //Search first in static table
    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        const char *table_name = hpack_static_table.name_table[i];
        if (strncmp(table_name, name, strlen(name)) == 0) {
            return i + 1;
        }
    }
    //Then search in dynamic table
    char tmp_name[MAX_HEADER_NAME_LEN];
    for (uint8_t i = 0; i < hpack_tables_dynamic_table_length(dynamic_table); i++) {
        hpack_tables_dynamic_find_entry_name(dynamic_table, i + HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name);
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
