#include "http.h"
#include "two.h"
#include "unit.h"

extern char *http_get_method(char *method);

void setUp(void) {}

int hello_world(char *method, char *uri, char *response, unsigned int maxlen)
{
    (void)method;
    (void)uri;

    char *msg = "Hello, World!!!";
    int len   = MIN(strlen(msg), maxlen);

    // Copy the response
    memcpy(response, msg, len);
    return len;
}

void test_http_get_method(void)
{
    TEST_ASSERT_EQUAL_STRING("GET", http_get_method("GET"));
    TEST_ASSERT_EQUAL_STRING("POST", http_get_method("POST"));
    TEST_ASSERT_EQUAL_STRING("HEAD", http_get_method("HEAD"));
    TEST_ASSERT_EQUAL_STRING("PUT", http_get_method("PUT"));
    TEST_ASSERT_EQUAL_STRING("DELETE", http_get_method("DELETE"));
    TEST_ASSERT_EQUAL(NULL, http_get_method("CONNECT"));
}

void test_http_get(void) {}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_http_get_method);

    return UNITY_END();
}
