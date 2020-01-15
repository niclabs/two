#include <stdio.h>
#include <errno.h>

#include "unit.h"
#include "header_list.h"

void setUp(void)
{}

void tearDown(void)
{}

void test_header_list(void){
	// initialize headers
	header_list_t headers;
	header_list_reset(&headers);

    int res = header_list_add(&headers, "hello", "goodbye");
    int size = strlen("hello") + strlen("goodbye") + 2; // 14

    // test adding a new header when empty
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(size, header_list_size(&headers)); // hello + 1 + goodbye + 1 + padding (4)
	TEST_ASSERT_EQUAL(1, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("goodbye", header_list_get(&headers, "hello"));

    // add the new header at the end
    res = header_list_add(&headers, "hi", "bye");
    size += strlen("hi") + strlen("bye") + 2; // 21

	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    // addding to an existing header
    // will shift the memory if there is enough available
    res = header_list_add(&headers, "hello", "world");
    size += strlen("world") + 1; // + ',' + "world"  = 27

	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("goodbye,world", header_list_get(&headers, "hello"));

    // add to an existing header when no more memory is available 
    // should return -1
    res = header_list_add(&headers, "hi", "blue ivy");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 on failed write");
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    // addding to an existing header
    // will shift the memory if there is enough available
    res = header_list_add(&headers, "hi", "baby");
    size += strlen("baby") + 1; // + ',' + "baby"  = 32
        
	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));
	TEST_ASSERT_EQUAL(2, header_list_count(&headers));
    TEST_ASSERT_EQUAL_STRING("bye,baby", header_list_get(&headers, "hi"));

    // addding to an existing header
    // when buffer is full
    res = header_list_add(&headers, "hi", "blue");
    TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 on failed write");
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    // get a non-existing header should return NULL
    TEST_ASSERT_EQUAL(NULL, header_list_get(&headers,"here's"));

    // set a new header when no more space is available should return -1
    res = header_list_set(&headers, "here's", "johnny");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 on failed write");
    TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    // setting a smaller header should free space
    res = header_list_set(&headers, "hello", "girl");
    size -= strlen("goodbye,world");
    size += strlen("girl"); // 32 - 13 + 4 = 23

	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
    TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));
    TEST_ASSERT_EQUAL_STRING("girl", header_list_get(&headers, "hello"));

    // setting a header when no more space is available should return an error
    res = header_list_set(&headers, "hello", "my baby my baby my baby");
	TEST_ASSERT_EQUAL_MESSAGE(-1, res, "add header should return -1 on failed write");
    TEST_ASSERT_EQUAL(2, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    // set a new header space is available should return 0
    res = header_list_set(&headers, "i'm", "here");
    size += strlen("i'm") + 1 + strlen("here") + 1; // 23 + 3 + 1 + 4 + 1 = 32

	TEST_ASSERT_EQUAL_MESSAGE(0, res, "add header should return 0 on succesful write");
    TEST_ASSERT_EQUAL(3, header_list_count(&headers));
	TEST_ASSERT_EQUAL(size, header_list_size(&headers));

    int count = header_list_count(&headers);
    http_header_t hlist[count];
    header_list_all(&headers, hlist);

    // test the final ordering of the list
    TEST_ASSERT_EQUAL_STRING("hi", hlist[0].name);
    TEST_ASSERT_EQUAL_STRING("bye,baby", hlist[0].value);
    TEST_ASSERT_EQUAL_STRING("hello", hlist[1].name);
    TEST_ASSERT_EQUAL_STRING("girl", hlist[1].value);
    TEST_ASSERT_EQUAL_STRING("i'm", hlist[2].name);
    TEST_ASSERT_EQUAL_STRING("here", hlist[2].value);

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
