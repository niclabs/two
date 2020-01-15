#include <stdio.h>
#include <errno.h>

#include "unit.h"
#include "headers.h"

void setUp(void)
{}

void tearDown(void)
{}

void test_header_list(void){
	// initialize headers
	header_list_t headers;
	header_list_reset(&headers);

    int res = header_list_add(&headers, "hello", "goodbye");

    // test adding a new header when empty
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(18, header_list_size(&headers)); // hello + 1 + goodbye + 1 + padding (4)
	TEST_ASSERT_EQUAL(1, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("goodbye", header_list_get(&headers, "hello"));

    // add the new header at the end
    res = header_list_add(&headers, "hi", "bye");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(29, header_list_size(&headers));

    // add the new header, when padding is not enough
    // will shift the memory if there is enough available
    res = header_list_add(&headers, "hello", "world");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("goodbye,world", header_list_get(&headers, "hello"));

    // add the new header, when padding is not enough and
    // there is not enough memory will return -1
    res = header_list_add(&headers, "hi", "joe");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 when no more space is available");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));

    // add the new header when the padding is enough
    // will add it using the padding
    res = header_list_add(&headers, "hi", "jo");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("bye,jo", header_list_get(&headers, "hi"));

    // set a header when there is enough space to fit the new value
    res = header_list_set(&headers, "hi", "johnny");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("johnny", header_list_get(&headers, "hi"));

    // set a header when there is not enough space
    res = header_list_set(&headers, "hello", "goodbye my sweet prince");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 when no more space is available");
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("goodbye,world", header_list_get(&headers, "hello"));

    // set a header when there is enough space should free space
    res = header_list_set(&headers, "hello", "baby");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(26, header_list_size(&headers));

    // set a header when there is enough space at the end should
    // shift the memory
    res = header_list_set(&headers, "hi", "my friend");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("my friend", header_list_get(&headers, "hi"));

    // setting a smaller header should free size
    res = header_list_set(&headers, "hello", "be");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(30, header_list_size(&headers));

    // setting a smaller header should free size
    res = header_list_set(&headers, "hi", "jo");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(25, header_list_size(&headers));

    // set a non existing header
    res = header_list_set(&headers, "sup", "x");
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(3, header_list_count(&headers));
	TEST_ASSERT_EQUAL(32, header_list_size(&headers));

    int count = header_list_count(&headers);
    http_header_t hlist[count];
    header_list_all(&headers, hlist);

    TEST_ASSERT_EQUAL_STRING("hello", hlist[0].name);
    TEST_ASSERT_EQUAL_STRING("be", hlist[0].value);
    TEST_ASSERT_EQUAL_STRING("hi", hlist[1].name);
    TEST_ASSERT_EQUAL_STRING("jo", hlist[1].value);
    TEST_ASSERT_EQUAL_STRING("sup", hlist[2].name);
    TEST_ASSERT_EQUAL_STRING("x", hlist[2].value);

    // test that the header list values are reset to zero
    header_list_reset(&headers);
	TEST_ASSERT_EQUAL(0, header_list_count(&headers));
	TEST_ASSERT_EQUAL(0, header_list_size(&headers));
}


int main(void)
{
    UNIT_TESTS_BEGIN();
	UNIT_TEST(test_header_list);
    UNIT_TESTS_END();
}
