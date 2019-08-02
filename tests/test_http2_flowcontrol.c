#include <errno.h>
#include "unit.h"
#include "fff.h"
#include "http2_flowcontrol.h"
#include "logging.h"


void setUp(void)
{
  //FFF_RESET_HISTORY();
}

void test_get_window_available_size(void){
  h2_window_manager_t window;
  window.window_size = 20;
  window.window_used = 10;
  uint32_t size = get_window_available_size(window);
  TEST_ASSERT_MESSAGE(size == 10, "Available size must be 10");
}

void test_increase_window_used(void){
  h2_window_manager_t window;
  window.window_used = 0;
  int rc = increase_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 0, "Size of window used must be 0");
  rc = increase_window_used(&window, 10);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 10, "Size of window used must be 10");
  rc = increase_window_used(&window, 0);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 10, "Size of window used must be 10");
  rc = increase_window_used(&window, 300);
  TEST_ASSERT_MESSAGE(rc == 0, "Return code must be 0");
  TEST_ASSERT_MESSAGE(window.window_used == 310, "Size of window used must be 310");
}
int main(void)
{
  UNIT_TESTS_BEGIN();
  UNIT_TEST(test_get_window_available_size);
  UNIT_TEST(test_increase_window_used);
  return UNIT_TESTS_END();
}
