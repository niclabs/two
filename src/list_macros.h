#ifndef LIST_MACROS_H_
#define LIST_MACROS_H_

/**
 * Generic list operation macros, a list is any
 * struct where the first element in the structure
 * is a pointer to the next element in the list
 * *IMPORTANT* it must be the first element or the operations
 * WILL fail with a segmentation fault
 *
 * struct s {
 *      struct s * next;
 *
 *      // other variable definitions
 *      int one;
 *      char c;
 * };
 */

/**
 * Private generic list item.
 */
struct __li {
    void *next;
};

// Get next element of the list
#define __NEXT(elem) ((struct __li *)elem)->next

// Push an element at the beginning of the list
#define LIST_PUSH(elem, list)       \
    ({                              \
        void *next = (elem)->next;  \
        (elem)->next = list;        \
        list = (elem);              \
        next;                       \
    })

// Append an element at the end of the list
#define LIST_APPEND(elem, list)                          \
    ({                                                   \
        void *curr = list;                               \
        while (curr != NULL && __NEXT(curr) != NULL)     \
        {                                                \
            curr = __NEXT(curr);                         \
        }                                                \
        if (curr == NULL) {                              \
            list = (elem);                               \
        }                                                \
        else {                                           \
            __NEXT(curr) = (elem);                       \
        }                                                \
        (elem);                                          \
    })

// Pop the first element of the list
#define LIST_POP(list)          \
    ({                          \
        void *elem = list;      \
        if (list != NULL) {     \
            list = list->next;  \
        }                       \
        elem;                   \
    })

// Count the number of elements of the list
#define LIST_COUNT(list)                \
    ({                                  \
        int count = 0;                  \
        void *elem = list;              \
        while (elem != NULL) {          \
            count++;                    \
            elem = __NEXT(elem);        \
        }                               \
        count;                          \
    })

// Use this macro for comparisons in LIST_FIND
// e.g. f = LIST_FIND(list, LIST_ELEM(struct type)->num) == 1);
// will find the first element with variable num == 1
#define LIST_ELEM(type) ((type *)elem)

// Find the first element that matches condition.
// Use with LIST_ELEM
#define LIST_FIND(list, condition)          \
    ({                                      \
        void *elem = list;                  \
        void *res = NULL;                   \
        while (elem != NULL) {              \
            if (condition) {                \
                res = elem;                 \
                break;                      \
            }                               \
            elem = __NEXT(elem);            \
        }                                   \
        res;                                \
    })

// Delete the given element from the list
#define LIST_DELETE(elem, list)                         \
    ({                                                  \
        void *res = NULL;                               \
        void *curr = list, *prev = NULL;                \
        while (curr != NULL) {                          \
            if (curr == (elem)) {                       \
                if (prev == NULL) {                     \
                    list = __NEXT(curr);                \
                }                                       \
                else {                                  \
                    __NEXT(prev) = __NEXT(curr);        \
                }                                       \
                res = curr;                             \
                break;                                  \
            }                                           \
            prev = curr;                                \
            curr = __NEXT(curr);                        \
        }                                               \
        res;                                            \
    })

#endif
