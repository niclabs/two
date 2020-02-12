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

char *content_type_text_plain(char *content_type)
{
    if (strcmp(content_type, "text/plain") == 0) {
        return content_type;
    }
    return NULL;
}

int resource_with_error(char *method, char *uri, char *response,
                        unsigned int maxlen)
{
    (void)method;
    (void)uri;
    (void)response;
    (void)maxlen;
    return -1;
}

int hello_world_bad_length(char *method, char *uri, char *response,
                           unsigned int maxlen)
{
    (void)method;
    (void)uri;

    char *msg = "Hello, World!!!";
    int len   = MIN(strlen(msg), maxlen);

    // Copy the response
    memcpy(response, msg, len);
    return 35;
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
    TEST_ASSERT_EQUAL(0, http_has_method_support("HEAD"));
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
    content_type_allowed_fake.custom_fake = content_type_text_plain;

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

void test_http_error(void)
{
    char content[32];
    http_response_t res = { .content = content };

    http_error(&res, 404);
    TEST_ASSERT_EQUAL(404, res.status);
    TEST_ASSERT_EQUAL(0, res.content_length);

    http_error(&res, 500);
    TEST_ASSERT_EQUAL(500, res.status);
    TEST_ASSERT_EQUAL(0, res.content_length);
}

void test_http_handle_request(void)
{
    content_type_allowed_fake.custom_fake = content_type_text_plain;

    // register resource
    TEST_ASSERT_EQUAL(
      0, two_register_resource("GET", "/", "text/plain", hello_world));
    TEST_ASSERT_EQUAL(0, two_register_resource("GET", "/bad", "text/plain",
                                               hello_world_bad_length));
    TEST_ASSERT_EQUAL(0, two_register_resource("GET", "/err", "text/plain",
                                               resource_with_error));

    // prepare request and response
    char content[32];
    http_response_t res = { .content = content };
    http_request_t req  = { .method         = "GET",
                           .path           = "/index.html",
                           .headers_length = 0 };

    // request invalid path
    http_handle_request(&req, &res, 32);
    TEST_ASSERT_EQUAL(404, res.status);
    TEST_ASSERT_EQUAL(0, res.content_length);

    // request unsupported method
    req.method = "POST";
    req.path   = "/";
    http_handle_request(&req, &res, 32);
    TEST_ASSERT_EQUAL(501, res.status);
    TEST_ASSERT_EQUAL(0, res.content_length);

    // request ok
    req.method = "GET";
    req.path   = "/";
    http_handle_request(&req, &res, 32);
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(15, res.content_length);
    TEST_ASSERT_EQUAL_STRING("Hello, World!!!", res.content);

    // check that content length is not above maxlen
    req.method = "GET";
    req.path   = "/bad";
    http_handle_request(&req, &res, 32);
    TEST_ASSERT_EQUAL(200, res.status);
    TEST_ASSERT_EQUAL(32, res.content_length);
    TEST_ASSERT_EQUAL_STRING("Hello, World!!!", res.content);

    // check 500 error
    req.method = "GET";
    req.path   = "/err";
    http_handle_request(&req, &res, 32);
    TEST_ASSERT_EQUAL(500, res.status);
    TEST_ASSERT_EQUAL(0, res.content_length);
}

int main(void)
{
    UNIT_TESTS_BEGIN();

    UNIT_TEST(test_http_get_method);
    UNIT_TEST(test_http_has_method_support);
    UNIT_TEST(test_is_valid_path);
    UNIT_TEST(test_parse_uri);
    UNIT_TEST(test_resources);
    UNIT_TEST(test_http_error);
    UNIT_TEST(test_http_handle_request);

    return UNIT_TESTS_END();
}
