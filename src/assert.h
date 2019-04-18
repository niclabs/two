#ifndef ASSERT_H
#define ASSERT_H

#include <stdlib.h>
#include <stdio.h>

#ifdef TEST /* For unit testing */
#define ASSERT(x) ((void)0)
#else
#define ASSERT_STR(x) #x
#define ASSERT(x)                                                                                                                            \
    if (!(x))                                                                                                                                \
    {                                                                                                                                        \
        fprintf(stderr, "Assertion failed: (%s), file %s, function %s, line %d.\n", ASSERT_STR(x), __FILE__, __PRETTY_FUNCTION__, __LINE__); \
        abort();                                                                                                                             \
    }
#endif
#define assert(x) ASSERT(x)

#endif /* ASSERT_H */