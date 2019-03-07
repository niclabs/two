#include <errno.h>
#include <stdio.h>

#include "common.h"
#include "lwip.h"
#include "lwip/netif.h"
#include "net/ipv6/addr.h"
#include "shell.h"
#include "msg.h"

#define MAIN_QUEUE_SIZE (8)
static msg_t _main_msg_queue[MAIN_QUEUE_SIZE];

static int ifconfig(int argc, char **argv)
{
    (void)argc;
    (void)argv;
    for (struct netif *iface = netif_list; iface != NULL; iface = iface->next)
    {
        printf("%s_%02u: ", iface->name, iface->num);
#ifdef MODULE_LWIP_IPV6
        char addrstr[IPV6_ADDR_MAX_STR_LEN];
        for (int i = 0; i < LWIP_IPV6_NUM_ADDRESSES; i++)
        {
            if (!ipv6_addr_is_unspecified((ipv6_addr_t *)&iface->ip6_addr[i]))
            {
                printf(" inet6 %s\n", ipv6_addr_to_str(addrstr, (ipv6_addr_t *)&iface->ip6_addr[i],
                                                       sizeof(addrstr)));
            }
        }
#endif
        puts("");
    }
    return 0;
}

static const shell_command_t shell_commands[] = {
    {"echo-server", "Echo server", echo_server},
    {"echo-client", "Echo client", echo_client},
    {"ifconfig", "Shows assigned IPv6 addresses", ifconfig},
    {NULL, NULL, NULL}};

static char line_buf[SHELL_DEFAULT_BUFSIZE];

int main(void)
{
    /* we need a message queue for the thread running the shell in order to
     * receive potentially fast incoming networking packets */
    msg_init_queue(_main_msg_queue, MAIN_QUEUE_SIZE);

    puts("RIOT lwip test application");
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}