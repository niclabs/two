#include <stdio.h>
#include <errno.h>

#include "unit.h"
#include "headers.h"

void setUp(void)
{}

void tearDown(void)
{}

void test_headers(void)
{
	int res;
	char long_value[32];
	char value[16];
	char short_value[4];

	header_t hlist[3];
	// initialize headers
	headers_t headers;

	headers_init(&headers, hlist, 3);

	TEST_ASSERT_EQUAL_MESSAGE(0, headers_count(&headers), "header size should equal to 0 before first succesful write");

	// test header name too large
	res = headers_set(&headers, "hello world", "goodbye");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if header name is too large");
	TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header name is too large");
	TEST_ASSERT_EQUAL_MESSAGE(0, headers_count(&headers), "header size should equal to 0 before first succesful write");

	// test header value too large
	res = headers_set(&headers, "hello", "goodbye my sweet prince");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 if header value is too large");
	TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header value is too large");

	// test succesful write
	res = headers_set(&headers, "hello", "goodbye");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should equal to 1 after first write");

	// Check read
	res = headers_get(&headers, "hello", value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("goodbye", value, "read of existing header should set correct value");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should remain constant after a read");

	// test write an already existing key
	res = headers_set(&headers, "hello", "it's me");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should remain constant after writing an existing key");

	// Check read with different case
	res = headers_get(&headers, "HeLlo", value);
	TEST_ASSERT_EQUAL_STRING_MESSAGE("it's me", value, "read of header should be case insensitive");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should remain constant after a read");

	// test succesful write
	res = headers_set(&headers, "Hey jude", "remember");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(2, headers_count(&headers), "header size should equal to 2 after second write");

	// Check read into larger array
	res = headers_get(&headers, "Hey jude", long_value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("remember", long_value, "read of existing header should write to buf with correct strlen");
	TEST_ASSERT_EQUAL_MESSAGE(2, headers_count(&headers), "header size should remain constant after a read");

	// test succesful write
	res = headers_set(&headers, "sad song", "make it better");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should equal to 3 after third write");

	// Check read into shorter array
	res = headers_get(&headers, "sad song", short_value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "succesful read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("make it better", short_value, "read of existing header into shorter array should truncate the value");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should remain constant after a read");

	// Check read into a full list
	res = headers_get(&headers, "sad song", long_value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("make it better", long_value, "read of existing header should write to buf with correct strlen");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should remain constant after a read");

	// test write after header size
	res = headers_set(&headers, "better", "Na na na naa-naa");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "set header should return -1 when headers list is full");
	TEST_ASSERT_EQUAL_MESSAGE(ENOMEM, errno, "errno should be set to ENOMEM when trying to write after list is full");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should remain constant after an error write");

	// test write an already existing key after header list is full
	res = headers_set(&headers, "hey jude", "don't be afraid");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "set header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should remain constant after writing an existing key");

	// Check read into already written array
	res = headers_get(&headers, "hey jude", value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("don't be afraid", value, "read of existing header should with correct strlen");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "header size should remain constant after a read");

	// Check read of a non existing key
	res = headers_get(&headers, "yesterday", value);
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "read of non existing header should return -1");
}

void test_headers_add(void)
{
	int res;
	char value[16];

	header_t hlist[3];
	// initialize headers
	headers_t headers;

	headers_init(&headers, hlist, 3);

	TEST_ASSERT_EQUAL_MESSAGE(0, headers_count(&headers), "header size should equal to 0 before first succesful write");
	
	// test header name too large
	res = headers_add(&headers, "hello world", "goodbye");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 if header name is too large");
	TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header name is too large");
	TEST_ASSERT_EQUAL_MESSAGE(0, headers_count(&headers), "header size should equal to 0 before first succesful write");

	// test header value too large
	res = headers_add(&headers, "hello", "goodbye my sweet prince");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 if header value is too large");
	TEST_ASSERT_EQUAL_MESSAGE(EINVAL, errno, "errno should be set to EINVAL if header value is too large");
	TEST_ASSERT_EQUAL_MESSAGE(0, headers_count(&headers), "header size should equal to 0 before first succesful write");

	// test succesful write
	res = headers_add(&headers, "hello", "goodbye");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should equal to 1 after first write");

	// Check read
	res = headers_get(&headers, "hello", value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("goodbye", value, "read of existing header should set correct value");
	TEST_ASSERT_EQUAL_MESSAGE(1, headers_count(&headers), "header size should remain constant after a read");

	// test succesful write
	res = headers_add(&headers, "hello", "it's me");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(2, headers_count(&headers), "add header should always increase size");

	// Check read
	res = headers_get(&headers, "hello", value);
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "read of existing header should return 0");
	TEST_ASSERT_EQUAL_STRING_MESSAGE("goodbye", value, "read of existing header should return first value added");

	// test succesful write
	res = headers_add(&headers, "hello", "my baby");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "add header should always increase size");

	// test succesful write
	res = headers_add(&headers, "hello", "my darling");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 if array is full");
	TEST_ASSERT_EQUAL_MESSAGE(ENOMEM, errno, "errno should be set to ENOMEM when trying to write after list is full");
	TEST_ASSERT_EQUAL_MESSAGE(3, headers_count(&headers), "add header should maintain size after a failure");
}


int main(void)
{
    UNIT_TESTS_BEGIN();
    UNIT_TEST(test_headers);
    UNIT_TEST(test_headers_add);
    UNIT_TESTS_END();
}
