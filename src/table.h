#ifndef TABLE_H
#define TABLE_H

// Key value pair
typedef struct TABLE_ENTRY {
    char name [32];
    char value [128];
} table_pair_t;


// Key value pair
typedef struct KEY_POINTER_MAP {
    char name [64];
    void * ptr;
} key_pointer_map_t;

#endif /* TABLE_H */
