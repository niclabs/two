#include "unit.h"

#include "fff.h"

#include "sock_non_blocking.h"
#include "net.h"

DEFINE_FFF_GLOBALS;
FAKE_VALUE_FUNC(int, sock_create, sock_t*);
FAKE_VALUE_FUNC(int, sock_listen, sock_t*, uint16_t);
FAKE_VALUE_FUNC(int, sock_poll, sock_t*);
FAKE_VALUE_FUNC(int, sock_read, sock_t*, char*, unsigned int);
FAKE_VALUE_FUNC(int, sock_write, sock_t*, char*, unsigned int);
FAKE_VALUE_FUNC(int, sock_accept, sock_t*, sock_t*);

#define FFF_FAKES_LIST(FAKE)            \
    FAKE(sock_accept)                   \
    FAKE(sock_listen)                   \
    FAKE(sock_poll)                     \
    FAKE(sock_read)                     \
    FAKE(sock_write)                    \
    FAKE(sock_accept)

void setUp()
{
    /* Register resets */
    FFF_FAKES_LIST(RESET_FAKE);

    /* reset common FFF internal structures */
    FFF_RESET_HISTORY();
}

void test_net_client_connect(void)
{
    // TODO: Implement unit tests for net
}

int main(void)
{
    UNITY_BEGIN();

    UNIT_TEST(test_net_client_connect);

    return UNITY_END();
}
