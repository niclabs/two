#ifndef LL_MACROS_H_
#define LL_MACROS_H_

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
struct __ll {
    void *next;
};

// Get next element of the list
#define __NEXT(elem) ((struct __ll *)elem)->next
#define __LL_CONCAT1(a, b) a##b
#define __LL_CONCAT(a, b) __LL_CONCAT1(a, b)

#define LL(type, name, maxlen)              \
    type __LL_CONCAT(name,_list)[maxlen];   \
    type *name

#define LL_STATIC(type, name, maxlen)               \
    static type __LL_CONCAT(name,_list)[maxlen];    \
    static type *name

#define LL_INIT(name, maxlen)                                                   \
    do {                                                                        \
        memset(__LL_CONCAT(name,_list), 0, sizeof(__LL_CONCAT(name,_list)));    \
        for (int i = 0; i < maxlen - 1; i++) {                                  \
            __LL_CONCAT(name,_list)[i].next = &__LL_CONCAT(name,_list)[i + 1];  \
        }                                                                       \
        name = &__LL_CONCAT(name,_list)[0];                                     \
    } while(0)

// Move the first element (if any) from the source list into
// the end of the destination list
#define LL_MOVE(src, dst)                   \
    ({                                      \
        void * elem = LL_POP(src);          \
        if (elem != NULL) {                 \
            memset(elem, 0, sizeof(*src));  \
            LL_APPEND(elem, dst);           \
        }                                   \
        elem;                               \
     })

// Move the first element (if any) from the source list into
// the beginning (push) of the destination list
#define LL_MOVEP(src, dst)                  \
    ({                                      \
        void * elem = LL_POP(src);          \
        if (elem != NULL) {                 \
            memset(elem, 0, sizeof(*src));  \
            LL_PUSH(elem, dst);             \
        }                                   \
        elem;                               \
     })

// Push an element at the beginning of the list
#define LL_PUSH(elem, list)         \
    ({                              \
        void *next = __NEXT(elem);  \
        __NEXT(elem) = list;        \
        list = (elem);              \
        next;                       \
    })

// Append an element at the end of the list
#define LL_APPEND(elem, list)                            \
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
#define LL_POP(list)            \
    ({                          \
        void *elem = list;      \
        if (list != NULL) {     \
            list = list->next;  \
        }                       \
        elem;                   \
    })

// Count the number of elements of the list
#define LL_COUNT(list)                  \
    ({                                  \
        int count = 0;                  \
        void *elem = list;              \
        while (elem != NULL) {          \
            count++;                    \
            elem = __NEXT(elem);        \
        }                               \
        count;                          \
    })

// Use this macro for comparisons in LL_FIND
// e.g. f = LL_FIND(list, LL_ELEM(struct type)->num) == 1);
// will find the first element with variable num == 1
#define LL_ELEM(type) ((type *)elem)

// Find the first element that matches condition.
// Use with LL_ELEM
#define LL_FIND(list, condition)            \
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
#define LL_DELETE(elem, list)                           \
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
