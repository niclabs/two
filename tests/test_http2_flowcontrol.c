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
int main(void)
{
  UNIT_TESTS_BEGIN();
  UNIT_TEST(test_get_window_available_size);
  return UNIT_TESTS_END();
}
