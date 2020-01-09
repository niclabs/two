#include "hpack/tables.h"
#include "headers.h"                                    /* for MAX_HEADER_NAME_LEN, MAX_HEADER_VALUE_LEN  */
#include <string.h>                                     /* for strlen, strncmp, memset*/
#include <assert.h>
#include "config.h"

#define LOG_MODULE LOG_MODULE_HPACK
#include "logging.h"

#define HPACK_TABLES_FIRST_INDEX_DYNAMIC 62

//HeaderPairs in static table

static hpack_static_table_t hpack_static_table = {
    .name_table = NAME_TABLE_LITERAL,
    .value_table = VALUE_TABLE_LITERAL,
};

/*
 * Function: hpack_tables_static_find_name_and_value
 * finds the entry in the given index and copies the result to name and value
 * Input:
 *      -> index: Index of the static table to retrieve
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 */
void hpack_tables_static_find_entry_name_and_value(uint8_t index, char *name, char *value)
{
    assert(index > 0);
    assert(index < 62);
    index--;                    //because static table begins at index 1
    char *table_name_pointer = hpack_static_table.name_table;
    char *table_value_pointer = hpack_static_table.value_table;

    for (uint8_t i = 0; i < index; i++) {
        size_t sz = strlen(table_name_pointer);
        table_name_pointer += sz + 1;

        size_t sz2 = strlen(table_value_pointer);
        table_value_pointer += sz2 + 1;
    }
    //printf("NAME %s VALUE %s \n", table_name_pointer, table_value_pointer);

    strncpy(name, table_name_pointer, strlen(table_name_pointer));
    strncpy(value, table_value_pointer, strlen(table_value_pointer));
    //printf("NAME %s VALUE %s \n", name, value);
}

/*
 * Function: hpack_tables_static_find_name_and_value
 * finds the entry in the given index and copies the result to name and value
 * Input:
 *      -> index: Index of the static table to retrieve
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 */
void hpack_tables_static_find_entry_name(uint8_t index, char *name)
{
    assert(index > 0);
    assert(index < 62);
    index--;
    char *table_name_pointer = hpack_static_table.name_table;

    for (uint8_t i = 0; i < index; i++) {
        size_t sz = strlen(table_name_pointer);
        table_name_pointer += sz + 1;
    }
    //printf("NAME %s \n", table_name_pointer);
    strncpy(name, table_name_pointer, strlen(table_name_pointer));
    //printf("NAME %s \n", name);
}

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_dynamic_pos_of_index
 * Finds the position in bytes of an index in dynamic table
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 * Output:
 *      Position in dynamic_table->buffer of entry... used like dynamic_table->buffer[(next-POS) % size], -2 if error
 */
int16_t hpack_tables_dynamic_pos_of_index(hpack_dynamic_table_t *dynamic_table, uint32_t index)
{
    assert(dynamic_table != NULL);
    assert(index > 61);

    uint16_t entries_counter = 0;
    uint16_t counter0 = 0;

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->next - i + dynamic_table->max_size) %
                                   dynamic_table->max_size]) {      // If char is 0, ENDSTR
            counter0++;
            if (counter0 % 2 == 0) {                                // name and value found
                entries_counter++;
            }
        }
        if (entries_counter == index - 61) { // index found!
            return i;
        }
    }
    //ERROR("Dynamic table is not initialized right, don't have 0's between words");
    return INTERNAL_ERROR;
}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_copy_to_ext
 * Copy string of table's buffer into external buffer
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> initial position: table's position of the entry
 *      -> char *buff: buffer where to copy
 * Output:
 *      Position of the end of the string in the table's buffer, -2 if error
 */
int16_t
hpack_tables_dynamic_copy_to_ext(hpack_dynamic_table_t *dynamic_table, int16_t initial_position, char *ext_buffer)
{
    assert(dynamic_table != NULL);
    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->next - initial_position + i + dynamic_table->max_size) %
                                   dynamic_table->max_size]) { //If char is 0, ENDSTR
            ext_buffer[i - 1] = 0;
            return initial_position - i;
        }
        //else copy
        ext_buffer[i - 1] = dynamic_table->buffer[
            (dynamic_table->next - initial_position + i + dynamic_table->max_size) % dynamic_table->max_size];

    }
    //if copy failed
    //ERROR("Dynamic table is not used right, can't find ENDSTR in buffer");
    return INTERNAL_ERROR;

}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_dynamic_compare_string
 * Compare a string in a buffer with a string in a position of the dynamic table buffer
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> initial position: table's current position
 *      -> char *buff: string to compare
 * Output:
 *      In case of equal strings, returns the end position of the string in the table's buffer, -1 otherwise
 */
int16_t
hpack_tables_dynamic_compare_string(hpack_dynamic_table_t *dynamic_table, uint16_t initial_position, char *buffer)
{
    assert(dynamic_table != NULL);
    int16_t max_size = (int16_t)strlen(buffer);

    for (uint16_t i = 0; i < max_size; i++) {
        if (buffer[i] !=
            dynamic_table->buffer[(dynamic_table->next - initial_position + 1 + i + dynamic_table->max_size) %
                                  dynamic_table->max_size]) {
            return -1; // different strings
        }
    }

    return (int16_t)(max_size + 1);   // plus the 0'
}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_copy_from_ext
 * Copy string of external buffer into table's buffer
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> initial position: table's position of the entry
 *      -> char *buff: buffer which has the string
 * Output:
 *      Position of the end of the string in the table's buffer, -2 if error
 */
int16_t hpack_tables_dynamic_copy_from_ext(hpack_dynamic_table_t *dynamic_table, int16_t initial_position,
                                           const char *ext_buffer)
{
    assert(dynamic_table != NULL);
    dynamic_table->buffer[(dynamic_table->next + initial_position + dynamic_table->max_size) %
                          dynamic_table->max_size] = 0; //set initial

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {

        if (!ext_buffer[i - 1]) { //If char is 0, ENDSTR
            dynamic_table->buffer[(dynamic_table->next + initial_position + i + dynamic_table->max_size) %
                                  dynamic_table->max_size] = 0;
            return initial_position + i;
        }
        //else copy
        dynamic_table->buffer[(dynamic_table->next + initial_position + i + dynamic_table->max_size) %
                              dynamic_table->max_size] = ext_buffer[i - 1];
    }
    //if copy failed
    //ERROR("String given don't have ENDSTR");
    return INTERNAL_ERROR;

}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: dynamic_table_pop
 * Delete the oldest element of the table
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 * Output:
 *      0 if success, -2 otherwise
 */
int8_t hpack_tables_dynamic_pop(hpack_dynamic_table_t *dynamic_table)
{
    assert(dynamic_table != NULL);
    uint16_t entry_length = 0;
    uint8_t counter0 = 0;

    if (dynamic_table->n_entries == 0) {
        ERROR("Trying to delete entry in dynamic table when it's empty");
        return INTERNAL_ERROR;
    }

    for (uint16_t i = 1; i < dynamic_table->max_size; i++) {
        if (!dynamic_table->buffer[(dynamic_table->first + i + dynamic_table->max_size) %
                                   dynamic_table->max_size]) { // if 0, ENDSTR

            counter0++;

            if (counter0 == 2) { //END
                dynamic_table->first = (dynamic_table->first + i + dynamic_table->max_size) % dynamic_table->max_size;
                dynamic_table->actual_size = (uint16_t)(dynamic_table->actual_size - (entry_length + 32u));
                dynamic_table->n_entries = (uint16_t)(dynamic_table->n_entries - 1u);
                return 0;
            }
        }
        else {
            entry_length++;
        }
    }
    //ERROR("Dynamic table is not initialized right, don't have 0's between words");
    return INTERNAL_ERROR;
}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_dynamic_find_entry_name_and_value
 * Finds entry in dynamic table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: Buffer to store name of the header
 *      -> *value: Buffer to store the value of the header
 * Output:
 *      0 if success, -1 in case of protocol error, -2 in case of internal error
 */
int8_t hpack_tables_dynamic_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name,
                                                      char *value)
{
    assert(dynamic_table != NULL);
    if (dynamic_table->n_entries < index - 61) { //CASE entry doesnt exist
        //ERROR("Decoding error: %d index is greater than the number of entries", index);
        return PROTOCOL_ERROR;
    }

    int16_t initial_position = hpack_tables_dynamic_pos_of_index(dynamic_table, index);
    if (initial_position < 0) {
        DEBUG("Error at finding position in buffer");
        return INTERNAL_ERROR;
    }

    //first copy name
    initial_position = hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, name);
    if (initial_position < 0) {
        DEBUG("Error while copying name into external buffer");
        return INTERNAL_ERROR;
    }

    //then copy value
    if (hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, value) < 0) {
        DEBUG("Error while copying value into external buffer");
        return INTERNAL_ERROR;
    }

    return 0;
}

#endif


#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_tables_dynamic_find_entry_name
 * Finds entry in dynamic table, entry is a pair name-value
 * Input:
 *      -> *dynamic_table: table which can be modified by server or client
 *      -> index: table's position of the entry
 *      -> *name: Buffer to store name of the header
 * Output:
 *      0 if success, -1 in case of protocol error, -2 in case of internal error
 */
int8_t hpack_tables_dynamic_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    assert(dynamic_table != NULL);
    if (dynamic_table->n_entries < index - 61) { //CASE entry doesnt exist
        //ERROR("Decoding error: %d index is greater than the number of entries", index);
        return PROTOCOL_ERROR;
    }

    int16_t initial_position = hpack_tables_dynamic_pos_of_index(dynamic_table, index);
    if (initial_position < 0) {
        DEBUG("Error at finding position in buffer");
        return INTERNAL_ERROR;
    }

    //only copy name
    if (hpack_tables_dynamic_copy_to_ext(dynamic_table, initial_position, name) < 0) {
        DEBUG("Error while copying name into external buffer");
        return INTERNAL_ERROR;
    }

    return 0;
}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/* Function: hpack_tables_dynamic_table_resize
 * Makes an update of the size of the dynamic table_length
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> settings_max_size: max size setted in settings frame at the beggining of the connection
 *      -> new_max_size: new virtual max size of the table, setted in the header of resize
 * Output:
 *      return 0 if the update is succesful, or -1 otherwise
 */

int8_t hpack_tables_dynamic_table_resize(hpack_dynamic_table_t *dynamic_table, uint32_t new_max_size)
{
    assert(dynamic_table != NULL);
    if (new_max_size > dynamic_table->settings_max_table_size) {
        ERROR("Decoding Error: Resize operation exceeds the maximum size set by the protocol ");
        return PROTOCOL_ERROR;
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
                dynamic_table->max_size = (uint16_t)new_max_size;

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
                    dynamic_table->buffer[(dynamic_table->next - i) + shift] = dynamic_table->buffer[(
                                                                                                         dynamic_table->next - i)];
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

    dynamic_table->max_size = (uint16_t)new_max_size;
    return 0;

}

#endif

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: dynamic_table_add_entry
 * Add an header pair entry in the table
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> *name: New entry name added
 *      -> *value: New entry value added
 * Output:
 *      0 if success, -1 in case of protocol error, -2 in case of internal error
 */
int8_t hpack_tables_dynamic_table_add_entry(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    assert(dynamic_table != NULL);
    uint16_t entry_size = (uint16_t)(strlen(name) + strlen(value) + 32);

    if (entry_size > dynamic_table->max_size) {
        DEBUG("New entry size exceeds the size of table ");
        return INTERNAL_ERROR; //entry's size exceeds the size of table
    }

    while (entry_size + dynamic_table->actual_size > dynamic_table->max_size) {
        hpack_tables_dynamic_pop(dynamic_table);
    }

    int16_t continue_pos = hpack_tables_dynamic_copy_from_ext(dynamic_table, 0, name); // copy name into table
    if (continue_pos < 0) {
        DEBUG("Error while adding name in table's buffer");
        return (int8_t)continue_pos;
    }
    continue_pos = hpack_tables_dynamic_copy_from_ext(dynamic_table, continue_pos, value); // copy value into table
    if (continue_pos < 0) {
        DEBUG("Error while adding value in table's buffer");
        return (int8_t)continue_pos;
    }

    dynamic_table->n_entries = (uint16_t)(dynamic_table->n_entries + 1u);
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
 *      0 if success,  -1 in case of protocol error, -2 in case of internal error
 */
int8_t hpack_tables_find_entry_name_and_value(hpack_dynamic_table_t *dynamic_table,
                                              uint32_t index,
                                              char *name,
                                              char *value)
{
    assert(index > 0);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        return hpack_tables_dynamic_find_entry_name_and_value(dynamic_table, index, name, value);
    }
    else {
        hpack_tables_static_find_entry_name_and_value((uint8_t)index, name, value);
    }
#else
    (void)dynamic_table;
    hpack_tables_static_find_entry_name_and_value(index, name, value);
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
 *      0 if success,  -1 in case of protocol error, -2 in case of internal error
 */
int8_t hpack_tables_find_entry_name(hpack_dynamic_table_t *dynamic_table, uint32_t index, char *name)
{
    assert(index > 0);
#if HPACK_INCLUDE_DYNAMIC_TABLE
    if (index >= HPACK_TABLES_FIRST_INDEX_DYNAMIC) {
        if (dynamic_table == NULL) {
            DEBUG("Dynamic table not initialized ");
            return INTERNAL_ERROR;
        }
        return hpack_tables_dynamic_find_entry_name(dynamic_table, index, name);
    }
    else {
        hpack_tables_static_find_entry_name((uint8_t)index, name);
    }
#else
    (void)dynamic_table;
    hpack_tables_static_find_entry_name(index, name);
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
 *      otherwise it returns -2.
 */
int hpack_tables_find_index(hpack_dynamic_table_t *dynamic_table, char *name, char *value)
{
    //Search first in static table
    char *table_name = hpack_static_table.name_table;
    char *table_value = hpack_static_table.value_table;

    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        if ((strlen(name) == strlen(table_name) && strncmp(table_name, name, strlen(name)) == 0) &&
            ((strlen(value) == strlen(table_value) && strncmp(table_value, value, strlen(value)) == 0))) {
            return i + 1;
        }
        table_name += strlen(table_name) + 1;
        table_value += strlen(table_value) + 1;
    }

#if HPACK_INCLUDE_DYNAMIC_TABLE
    //Then search in dynamic table with a linear search
    assert(dynamic_table != NULL);
    uint8_t strings_counter = 0;
    for (uint16_t i = 1; i <= dynamic_table->max_size; i++) {

        if (!dynamic_table->buffer[(dynamic_table->next - i + dynamic_table->max_size) % dynamic_table->max_size]) {
            //found an end of string
            strings_counter++;
            if (strings_counter % 2 == 0 && strings_counter > 0) {
                int16_t rc = hpack_tables_dynamic_compare_string(dynamic_table, i, name);
                if (rc > 0) { //match name
                    rc = hpack_tables_dynamic_compare_string(dynamic_table, i - rc, value);
                    if (rc > 0) {
                        return (strings_counter / 2) + HPACK_TABLES_FIRST_INDEX_DYNAMIC - 1;
                    }
                }
            }
        }

        if (strings_counter / 2 >= dynamic_table->n_entries) {
            break; //not found
        }
    }
#else
    (void)dynamic_table;
#endif
    DEBUG("Header with name '%s', and value '%s' not found", name, value);
    return INTERNAL_ERROR;
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
 *      otherwise it returns -2.
 */
int hpack_tables_find_index_name(hpack_dynamic_table_t *dynamic_table, char *name)
{

    //Search first in static table
    char *table_name = hpack_static_table.name_table;

    for (uint8_t i = 0; i < HPACK_TABLES_FIRST_INDEX_DYNAMIC; i++) {
        if (strlen(name) == strlen(table_name) && strncmp(table_name, name, strlen(name)) == 0) {
            return i + 1;
        }
        table_name += strlen(table_name) + 1;
    }

#if HPACK_INCLUDE_DYNAMIC_TABLE
    //Then search in dynamic table with a linear search
    assert(dynamic_table != NULL);
    uint8_t strings_counter = 0;
    for (uint16_t i = 1; i <= dynamic_table->max_size; i++) {

        if (!dynamic_table->buffer[(dynamic_table->next - i + dynamic_table->max_size) % dynamic_table->max_size]) {
            //found an end of string
            strings_counter++;
            if (strings_counter % 2 == 0 && strings_counter > 0) {
                int16_t rc = hpack_tables_dynamic_compare_string(dynamic_table, i, name);
                if (rc > 0) { //match name
                    return (strings_counter / 2) + HPACK_TABLES_FIRST_INDEX_DYNAMIC - 1;
                }
            }
        }

        if (strings_counter / 2 >= dynamic_table->n_entries) {
            break; //not found
        }
    }
#else
    (void)dynamic_table;
#endif
    DEBUG("Header with name '%s'not found", name);
    return INTERNAL_ERROR;
}

#if HPACK_INCLUDE_DYNAMIC_TABLE

/*
 * Function: hpack_init_dynamic_table
 * Initialize dynamic_table for protocol uses, but it requires a previous header_pair_t table initialization
 * Input:
 *      -> *dynamic_table: Dynamic table to search
 *      -> dynamic_table_max_size: Max size in bytes of new dynamic_table, it is set in HTTP SETTING-
 * Output:
 *     (void)
 */
void hpack_tables_init_dynamic_table(hpack_dynamic_table_t *dynamic_table, uint32_t dynamic_table_max_size)
{
    assert(dynamic_table != NULL);
    memset(dynamic_table->buffer, 0, dynamic_table_max_size);
    dynamic_table->max_size = (uint16_t)dynamic_table_max_size;
    dynamic_table->actual_size = 0;
    dynamic_table->n_entries = 0;
    dynamic_table->first = 0;
    dynamic_table->next = 0;
}

#endif
