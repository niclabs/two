#include "fff.h"
#include "http.h"
#include "two.h"
#include "unit.h"

typedef struct
{
    char path[TWO_MAX_PATH_SIZE];
    char *method;
    char *content_type;
    two_resource_handler_t handler;
} two_resource_t;

extern char *http_get_method(char *method);
extern int is_valid_path(char *path);
extern void parse_uri(char *uri, char *path, unsigned int maxpath,
                      char *query_params, unsigned int maxquery);
extern two_resource_t *find_resource(char *method, char *path);

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(char *, content_type_allowed, char *);

/* List of fakes used by this unit tester */
#define FFF_FAKES_LIST(FAKE) FAKE(content_type_allowed)

void setUp(void)
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

char *content_type_text(char *content_type)
{
    if (strcmp(content_type, "text/plain") == 0) {
        return content_type;
    }
    return NULL;
}

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

void test_http_has_method_support(void)
{
    TEST_ASSERT_EQUAL(1, http_has_method_support("GET"));
    TEST_ASSERT_EQUAL(1, http_has_method_support("HEAD"));
    TEST_ASSERT_EQUAL(0, http_has_method_support("POST"));
    TEST_ASSERT_EQUAL(0, http_has_method_support("PUT"));
    TEST_ASSERT_EQUAL(0, http_has_method_support("DELETE"));
    TEST_ASSERT_EQUAL(0, http_has_method_support("CONNECT"));
    TEST_ASSERT_EQUAL(0, http_has_method_support("INVALID"));
}

void test_is_valid_path(void)
{
    TEST_ASSERT_EQUAL(1, is_valid_path("/"));
    TEST_ASSERT_EQUAL(1, is_valid_path("/index.html"));
    TEST_ASSERT_EQUAL(1, is_valid_path("/folder/index.html"));
    TEST_ASSERT_EQUAL(1, is_valid_path("/a/b/index.html"));
    TEST_ASSERT_EQUAL(0, is_valid_path("index.html"));
}

void test_parse_uri(void)
{
    char path[32];
    char query_params[32];

    parse_uri("", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/", path);
    TEST_ASSERT_EQUAL_STRING("", query_params);

    parse_uri("/", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/", path);
    TEST_ASSERT_EQUAL_STRING("", query_params);

    parse_uri("index.html", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/index.html", path);
    TEST_ASSERT_EQUAL_STRING("", query_params);

    parse_uri("/a/index.html", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/a/index.html", path);
    TEST_ASSERT_EQUAL_STRING("", query_params);

    parse_uri("/index.cgi?a=1&b=2", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/index.cgi", path);
    TEST_ASSERT_EQUAL_STRING("a=1&b=2", query_params);

    parse_uri("index.cgi?a=1&b=2", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/index.cgi", path);
    TEST_ASSERT_EQUAL_STRING("a=1&b=2", query_params);

    parse_uri("?a=1&b=2", path, 32, query_params, 32);
    TEST_ASSERT_EQUAL_STRING("/", path);
    TEST_ASSERT_EQUAL_STRING("a=1&b=2", query_params);

    char short_path[4];
    char short_query[4];
    parse_uri("/index.html?a=1&b=2", short_path, 4, short_query, 4);
    TEST_ASSERT_EQUAL_STRING("/in", short_path);
    TEST_ASSERT_EQUAL_STRING("a=1", short_query);

    parse_uri("index.html?a=1&b=2", short_path, 4, short_query, 4);
    TEST_ASSERT_EQUAL_STRING("/in", short_path);
    TEST_ASSERT_EQUAL_STRING("a=1", short_query);
}

void test_resources(void)
{
    content_type_allowed_fake.custom_fake = content_type_text;

    // POST is not supported
    TEST_ASSERT_EQUAL(
      -1, two_register_resource("POST", "/", "text/plain", hello_world));
    TEST_ASSERT_EQUAL(NULL, find_resource("POST", "/"));

    // DELETE is not supported
    TEST_ASSERT_EQUAL(
      -1, two_register_resource("DELETE", "/", "text/plain", hello_world));
    TEST_ASSERT_EQUAL(NULL, find_resource("DELETE", "/"));

    // register with invalid path
    TEST_ASSERT_EQUAL(-1, two_register_resource("GET", "index.html",
                                                "text/plain", hello_world));
    TEST_ASSERT_EQUAL(NULL, find_resource("GET", "index.html"));

    // register with unsupported content type
    TEST_ASSERT_EQUAL(
      -1, two_register_resource("GET", "/", "text/markdown", hello_world));
    TEST_ASSERT_EQUAL(NULL, find_resource("GET", "/"));

    TEST_ASSERT_EQUAL(
      0, two_register_resource("GET", "/", "text/plain", hello_world));
    two_resource_t *res = find_resource("GET", "/");
    TEST_ASSERT_EQUAL(hello_world, res->handler);
}

void test_http_get(void) {}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_http_get_method);
    UNIT_TEST(test_http_has_method_support);
    UNIT_TEST(test_is_valid_path);
    UNIT_TEST(test_parse_uri);
    UNIT_TEST(test_resources);

    return UNITY_END();
}
