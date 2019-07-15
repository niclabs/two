#include <stdio.h>
#include <errno.h>

#include "unit.h"

#include "http_headers.h"

void setUp(void)
{
}

void tearDown(void)
{
}

void test_http_headers(void) {
    int res;
    char long_value[32];
    char value[16];
    char short_value[4];

    // initialize headers
    http_headers_t headers;
    http_headers_init(&headers);

    TEST_ASSERT_EQUAL_MESSAGE(0, http_headers_size(&headers), "header size should equal to 0 before first succesful write");
    
    // Tro to write to a NULL headers
    res = http_headers_set_header(NULL, "hello", "goodbye");
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if headers is null");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if headers is null ");

    // test header name too large
    res = http_headers_set_header(&headers, "hello world", "goodbye");
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if header name is too large");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header name is too large");
    TEST_ASSERT_EQUAL_MESSAGE(0, http_headers_size(&headers), "header size should equal to 0 before first succesful write");

    // test header value too large
    res = http_headers_set_header(&headers, "hello", "goodbye my sweet prince");
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if header value is too large");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header value is too large");
    TEST_ASSERT_EQUAL_MESSAGE(0, http_headers_size(&headers), "header size should equal to 0 before first succesful write");

    // test succesful write
    res = http_headers_set_header(&headers, "hello", "goodbye");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
    TEST_ASSERT_EQUAL_MESSAGE(1, http_headers_size(&headers), "header size should equal to 1 after first write");

    // test header name and value too large
    res = http_headers_set_header(&headers, "hello world", "goodbye my sweet prince");
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if header name or value is too large");
    TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header name or value is too large");
    TEST_ASSERT_EQUAL_MESSAGE(1, http_headers_size(&headers), "header size should remain constant after an error write");

    // Check read
    res = http_headers_get_header(&headers, "hello", value);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("goodbye", value, "read of existing header should set correct value");
    TEST_ASSERT_EQUAL_MESSAGE(1, http_headers_size(&headers), "header size should remain constant after a read");

    // test write an already existing key
    res = http_headers_set_header(&headers, "hello", "it's me");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
    TEST_ASSERT_EQUAL_MESSAGE(1, http_headers_size(&headers), "header size should remain constant after writing an existing key");

    // Check read with different case
    res = http_headers_get_header(&headers, "HeLlo", value);
    TEST_ASSERT_EQUAL_STRING_MESSAGE("it's me", value, "read of header should be case insensitive");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
    TEST_ASSERT_EQUAL_MESSAGE(1, http_headers_size(&headers), "header size should remain constant after a read");

    // test succesful write
    res = http_headers_set_header(&headers, "Hey jude", "remember");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
    TEST_ASSERT_EQUAL_MESSAGE(2, http_headers_size(&headers), "header size should equal to 2 after second write");

    // Check read into larger array
    res = http_headers_get_header(&headers, "Hey jude", long_value);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("remember", long_value, "read of existing header should write to buf with correct strlen");
    TEST_ASSERT_EQUAL_MESSAGE(2, http_headers_size(&headers), "header size should remain constant after a read");

    // test succesful write
    res = http_headers_set_header(&headers, "sad song", "make it better");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should equal to 3 after third write");

    // Check read into shorter array
    res = http_headers_get_header(&headers, "sad song", short_value);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "succesful read of existing header should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("make it better", short_value, "read of existing header into shorter array should truncate the value");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should remain constant after a read");

    // Check read into a full list
    res = http_headers_get_header(&headers, "sad song", long_value);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("make it better", long_value, "read of existing header should write to buf with correct strlen");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should remain constant after a read");

    // test write after header size
    res = http_headers_set_header(&headers, "better", "Na na na naa-naa");
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 when headers list is full");
    TEST_ASSERT_EQUAL_MESSAGE(ENOMEM, errno, "errno should be set to ENOMEM when trying to write after list is full");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should remain constant after an error write");

    // test write an already existing key after header list is full
    res = http_headers_set_header(&headers, "hey jude", "don't be afraid");
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should remain constant after writing an existing key");

    // Check read into already written array
    res = http_headers_get_header(&headers, "hey jude", value);
    TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
    TEST_ASSERT_EQUAL_STRING_MESSAGE("don't be afraid", value, "read of existing header should with correct strlen");
    TEST_ASSERT_EQUAL_MESSAGE(3, http_headers_size(&headers), "header size should remain constant after a read");

    // Check read of a non existing key
    res = http_headers_get_header(&headers, "yesterday", value);
    TEST_ASSERT_EQUAL_MESSAGE(-1, res, "read of non existing header should return -1");
}

int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_http_headers);
    UNIT_TESTS_END();
}
