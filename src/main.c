#include <errno.h>
#include <stdio.h>

#include "lwip.h"
#include "lwip/netif.h"
#include "net/ipv6/addr.h"
#include "msg.h"
#include "shell.h"

#define MAIN_MSG_QUEUE_SIZE (4)
static msg_t main_msg_queue[MAIN_MSG_QUEUE_SIZE];

extern int echo_cmd(int argc, char **argv);

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
    { "echo", "control echo server/client", echo_cmd },
    {"ifconfig", "Shows assigned IPv6 addresses", ifconfig},
    { NULL, NULL, NULL }
};

int main(void)
{
    /* a sendto() call performs an implicit bind(), hence, a message queue is
     * required for the thread executing the shell */
    msg_init_queue(main_msg_queue, MAIN_MSG_QUEUE_SIZE);
    puts("RIOT tcp echo example application");
    /* start shell */
    puts("All up, running the shell now");
    char line_buf[SHELL_DEFAULT_BUFSIZE];
    shell_run(shell_commands, line_buf, SHELL_DEFAULT_BUFSIZE);

    /* should be never reached */
    return 0;
}