#ifndef UNIT_H
#define UNIT_H

#include "logging.h"
#include "unity.h"

void setUp();
void tearDown(){}

#define UNIT_TEST(TestFunc)                       \
    {                                             \
        UNITY_NEW_TEST(#TestFunc)                 \
        if (TEST_PROTECT())                       \
        {                                         \
            DEBUG("Running %s", #TestFunc);        \
            setUp();                              \
            TestFunc();                           \
        }                                         \
        if (TEST_PROTECT() && (!TEST_IS_IGNORED)) \
        {                                         \
            DEBUG("Finishing %s", #TestFunc);      \
            tearDown();                           \
        }                                         \
        UnityConcludeTest();                      \
    }

#define UNIT_TESTS_BEGIN() UNITY_BEGIN()
#define UNIT_TESTS_END() UNITY_END()

#endif /* UNIT_H */