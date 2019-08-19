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
        return -2;
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
        return -2;
    }
    const char *table_name = hpack_static_table.name_table[index];
    strncpy(name, table_name, strlen(table_name));
    return 0;
}

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/*
 * Function: hpack_tables_dynamic_pos_of_index
 * Finds the position in bytes of an index in dynamic table
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 * Output:
 *      Position in dynamic_table->buffer of entry... used like dynamic_table->buffer[(next-POS) % size], -1 if error
 */
int16_t hpack_tables_dynamic_pos_of_index(hpack_dynamic_table_t *dynamic_table, uint32_t index)
{

    uint16_t entries_counter = 0;
    uint16_t counter0 = 0;

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->next - i + dynamic_table->max_size) % dynamic_table->max_size]) {    // If char is 0, ENDSTR
            counter0++;
            if (counter0 % 2 == 0) {                                                                                    // name and value found
                entries_counter++;
            }
        }
        if (entries_counter == index - 61) { // index found!
            return i;
        }
    }
    return -2;
}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/*
 * Function: hpack_tables_copy_to_ext
 * Copy string of table's buffer into external buffer
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> initial position: table's position of the entry
 *      -> char *buff: buffer where to copy
 * Output:
 *      Position of the end of the string in the table's buffer, -1 if error
 */
int16_t hpack_tables_dynamic_copy_to_ext(hpack_dynamic_table_t *dynamic_table, int16_t initial_position, char *ext_buffer)
{

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->next - initial_position + i + dynamic_table->max_size) % dynamic_table->max_size]) { //If char is 0, ENDSTR
            ext_buffer[i - 1] = 0;
            return initial_position - i;
        }
        //else copy
        ext_buffer[i - 1] = dynamic_table->buffer[(dynamic_table->next - initial_position + i + dynamic_table->max_size) % dynamic_table->max_size];

    }
    //if copy failed
    return -2;

}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/*
 * Function: hpack_tables_copy_from_ext
 * Copy string of external buffer into table's buffer
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> initial position: table's position of the entry
 *      -> char *buff: buffer which has the string
 * Output:
 *      Position of the end of the string in the table's buffer, -1 if error
 */
int16_t hpack_tables_dynamic_copy_from_ext(hpack_dynamic_table_t *dynamic_table, int16_t initial_position, char *ext_buffer)
{

    dynamic_table->buffer[(dynamic_table->next + initial_position + dynamic_table->max_size) % dynamic_table->max_size] = 0; //set initial

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {

        if (!ext_buffer[i - 1]) { //If char is 0, ENDSTR
            dynamic_table->buffer[(dynamic_table->next + initial_position + i + dynamic_table->max_size) % dynamic_table->max_size] = 0;
            return initial_position + i;
        }
        //else copy
        dynamic_table->buffer[(dynamic_table->next + initial_position + i + dynamic_table->max_size) % dynamic_table->max_size] = ext_buffer[i - 1];
    }
    //if copy failed
    return -2;

}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/*
 * Function: dynamic_table_pop
 * Delete the oldest element of the table
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 * Output:
 *      0 if success, -1 otherwise
 */
int8_t hpack_tables_dynamic_pop(hpack_dynamic_table_t *dynamic_table)
{

    uint16_t entry_length = 0;
    uint8_t counter0 = 0;

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->first + i + dynamic_table->max_size) % dynamic_table->max_size]) { // if 0, ENDSTR

            counter0++;

            if (counter0 == 2) { //END
                dynamic_table->first = (dynamic_table->first + i + dynamic_table->max_size) % dynamic_table->max_size;
                dynamic_table->actual_size = dynamic_table->actual_size - (entry_length + 32);
                dynamic_table->n_entries = dynamic_table->n_entries - 1;
                return 0;
            }
        }
        else {
            entry_length++;
        }
    }
    //ERROR CASE, not found 0
    return -2;
}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
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
    if (dynamic_table->n_entries < index - 61) { //CASE entry doesnt exist
        ERROR("Decoding error: %d index is greater than the number of entries", index);
        return -1;
    }

    int16_t initial_position = hpack_tables_dynamic_pos_of_index(dynamic_table, index);
    if (initial_position < 0) {
        DEBUG("Error at finding position in buffer");
        return -2;
    }

    //first copy name
    initial_position = hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, name);
    if (initial_position < 0) {
        DEBUG("Error while copying name into external buffer");
        return -2;
    }

    //then copy value
    if (hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, value) < 0) {
        DEBUG("Error while copying value into external buffer");
        return -2;
    }

    return 0;
}
#endif
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

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
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
    if (dynamic_table->n_entries < index - 61) { //CASE entry doesnt exist
        ERROR("Decoding error: %d index is greater than the number of entries", index);
        return -1;
    }

    int16_t initial_position = hpack_tables_dynamic_pos_of_index(dynamic_table, index);
    if (initial_position < 0) {
        ERROR("Error at finding position in buffer");
        return -2;
    }

    //only copy name
    if (hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, name) < 0) {
        ERROR("Error while copying name into external buffer");
        return -2;
    }

    return 0;
}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/* Function: hpack_tables_dynamic_table_resize
 * Makes an update of the size of the dynamic table_length
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> new_max_size: new virtual max size of the table, setted in the header of resize
 * Output:
 *      return 0 if the update is succesful, or -1 otherwise
 */

int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t new_max_size)
{
    if (new_max_size > HPACK_MAX_DYNAMIC_TABLE_SIZE) {
        ERROR("Decoding Error: Resize operation exceeds the maximum size set by the protocol ");
        return -1;
    }
    //delete old entries to fit in new size
    while (dynamic_table->actual_size > new_max_size) {
        hpack_tables_dynamic_pop(dynamic_table);
    }
    //now reorganize table
    if (dynamic_table->next <= dynamic_table->first) {
        if (dynamic_table->n_entries == 0) {
            dynamic_table->first = 0;
            dynamic_table->next = 0;
        }
        else {
            //two cases: [0, next] > [first, max_size] or else
            //always choose the smaller one
            if (dynamic_table->next <= (dynamic_table->max_size - dynamic_table->first)) {
                char aux_buffer[dynamic_table->next + 1];  //+1 for the zero -> end string
                //copy into aux
                for (uint16_t i = 0; i <= dynamic_table->next; i++) {
                    aux_buffer[i] = dynamic_table->buffer[i];
                }
                //shift
                uint16_t shift = dynamic_table->first;
                for (uint16_t i = 0; i < dynamic_table->max_size - dynamic_table->first; i++) {
                    dynamic_table->buffer[i] = dynamic_table->buffer[i + shift];
                }
                //now copy aux at the end
                uint16_t next_pos = dynamic_table->max_size - dynamic_table->first;
                for (uint16_t i = 0; i <= dynamic_table->next; i++) {
                    dynamic_table->buffer[i + next_pos] = aux_buffer[i];
                }
                dynamic_table->first = dynamic_table->first - shift;
                dynamic_table->next = next_pos + dynamic_table->next;
                dynamic_table->max_size = new_max_size;

            }
            else {
                char aux_buffer[dynamic_table->max_size - dynamic_table->first];
                //copy into aux
                for (uint16_t i = 0; i < dynamic_table->max_size - dynamic_table->first; i++) {
                    aux_buffer[i] = dynamic_table->buffer[dynamic_table->first + i];
                }
                //shift
                uint16_t shift = dynamic_table->max_size - dynamic_table->first;
                for (uint16_t i = 0; i <= dynamic_table->next; i++) {
                    //se copia de izquierda a derecha, para evitar sobreescribir al escribir a la derecha
                    dynamic_table->buffer[(dynamic_table->next - i) + shift] = dynamic_table->buffer[(dynamic_table->next - i)];
                }
                //now copy aux at the beginning
                for (uint16_t i = 0; i < dynamic_table->max_size - dynamic_table->first; i++) {
                    dynamic_table->buffer[i] = aux_buffer[i];
                }
                dynamic_table->first = 0;
                dynamic_table->next = shift + dynamic_table->next;

            }

        }
    }
    else {
        //SIMPLE SHIFT
        uint16_t shift = dynamic_table->first;
        //now shift in order
        for (uint16_t i = 0; i <= dynamic_table->next - dynamic_table->first; i++) {
            dynamic_table->buffer[i] = dynamic_table->buffer[shift + i]; //shifting...
        }
        dynamic_table->first = dynamic_table->first - shift;
        dynamic_table->next = dynamic_table->next - shift;

    }

    dynamic_table->max_size = new_max_size;
    return 0;

}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
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
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    uint16_t entry_size = (uint16_t)(strlen(name) + strlen(value) + 32);

    if (entry_size > dynamic_table->max_size) {
        DEBUG("New entry size exceeds the size of table ");
        return -2; //entry's size exceeds the size of table
    }

    while (entry_size + dynamic_table->actual_size > dynamic_table->max_size) {
        hpack_tables_dynamic_pop(dynamic_table);
    }

    int16_t continue_pos = hpack_tables_dynamic_copy_from_ext(dynamic_table, 0, name); // copy name into table
    if (continue_pos < 0) {
        DEBUG("Error while adding name in table's buffer");
        return continue_pos;
    }
    continue_pos = hpack_tables_dynamic_copy_from_ext(dynamic_table, continue_pos, value); // copy value into table
    if (continue_pos < 0) {
        DEBUG("Error while adding value in table's buffer");
        return continue_pos;
    }

    dynamic_table->n_entries = dynamic_table->n_entries + 1;
    dynamic_table->actual_size = dynamic_table->actual_size + entry_size;
    dynamic_table->next = (dynamic_table->next + continue_pos + dynamic_table->max_size) % dynamic_table->max_size;

    return 0;
}
#endif
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
    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }

        int8_t rc = hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, index, name, value);
        if (rc < 0) {
            DEBUG("The entry doesn't exist in dynamic table");
            return -2;
        }
    }
    else {
        int8_t rc = hpack_tables_static_find_entry_name_and_value(index, name, value);
        if (rc < 0) {
            DEBUG("The index was greater than the size of the static table");
            return -2;
        }
    }
    #else
    int8_t rc = hpack_tables_static_find_entry_name_and_value(index, name, value);
    if (rc < 0) {
        ERROR("The index was greater than the size of the static table");
        return -1;
    }
    #endif
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
    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            ERROR("Dynamic table not initialized ");
            return -1;
        }
        int8_t rc = hpack_tables_dynamic_find_entry_name(dynamic_table, index, name);
        if (rc < 0) {
            DEBUG("The entry doesn't exist in dynamic table");
            return -2;
        }
    }
    else {
        int8_t rc = hpack_tables_static_find_entry_name(index, name);
        if (rc < 0) {
            DEBUG("The index was greater than the size of the static table");
            return -2;
        }
    }
    #else
    int8_t rc = hpack_tables_static_find_entry_name(index, name);
    if (rc < 0) {
        ERROR("The index was greater than the size of the static table");
        return -1;
    }
    #endif
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
#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
#else
int hpack_tables_find_index(char *name, char *value)
#endif
{
    //Search first in static table
    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        const char *table_name = hpack_static_table.name_table[i];
        const char *table_value = hpack_static_table.value_table[i];
        if ((strlen(name) == strlen(table_name) && strncmp(table_name, name, strlen(name)) == 0) &&
            ((strlen(value) == strlen(table_value) && strncmp(table_value, value, strlen(value)) == 0))) {
            return i + 1;
        }
    }

    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    //Then search in dynamic table
    char tmp_name[MAX_HEADER_NAME_LEN];
    char tmp_value[MAX_HEADER_VALUE_LEN];
    for (uint16_t i = 0; i < dynamic_table->n_entries; i++) {
        hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, i + HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name, tmp_value);
        if ((strlen(tmp_name) == strlen(name) &&
             (strncmp(tmp_name, name, strlen(name)) == 0)) &&
            (strlen(tmp_value) == strlen(value) &&
             (strncmp(tmp_value, value, strlen(value)) == 0))) {
            return i + HPACK_TABLES_FIRST_INDEX_DYNAMIC;
        }
    }
    #endif
    return -2;
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
#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name)
#else
int hpack_tables_find_index_name(char *name)
#endif
{

    //Search first in static table
    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        const char *table_name = hpack_static_table.name_table[i];
        if (strlen(name) == strlen(table_name) && strncmp(table_name, name, strlen(name)) == 0) {
            return i + 1;
        }
    }

    #ifdef HPACK_INCLUDE_DYNAMIC_TABLE
    //Then search in dynamic table, TODO this parts can be optimized a lot!,
    //right now it copies one on one values from table to buffer, it can do instead a linear search in the buffer;
    char tmp_name[MAX_HEADER_NAME_LEN];
    for (uint8_t i = 0; i < dynamic_table->n_entries; i++) {
        hpack_tables_dynamic_find_entry_name(dynamic_table, i + HPACK_TABLES_FIRST_INDEX_DYNAMIC, tmp_name);

        if (strlen(name) == strlen(tmp_name) && strncmp(tmp_name, name, strlen(name)) == 0) {
            return i + HPACK_TABLES_FIRST_INDEX_DYNAMIC;
        }
    }
    #endif
    return -2;
}

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
uint32_t hpack_tables_get_table_length(uint32_t dynamic_table_size)
{
    return (uint32_t)((dynamic_table_size / 32) + 1);
}
#endif

#ifdef HPACK_INCLUDE_DYNAMIC_TABLE
/*
 * Function: hpack_init_dynamic_table
 * Initialize dynamic_table for protocol uses, but it requires a previous header_pair_t table initialization
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> dynamic_table_max_size: Max size in bytes of new dynamic_table, it is set in HTTP SETTING-
 * Output:
 *     0 if success
 */
int8_t hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size, char *buffer)
{
    memset(dynamic_table, 0, sizeof(hpack_dynamic_table_t));
    dynamic_table->max_size = dynamic_table_max_size;
    dynamic_table->n_entries = 0;
    dynamic_table->first = 0;
    dynamic_table->next = 0;
    dynamic_table->buffer = buffer;
    return 0;
}
#endif
