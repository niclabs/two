/* needed for posix usleep */
#ifndef _XOPEN_SOURCE
#define _XOPEN_SOURCE 600
#endif

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

#include "thread.h"

#define ENABLE_DEBUG  (1)
#define SERVER_MSG_QUEUE_SIZE (8)
#define SERVER_BUFFER_SIZE (64)

static int server_sock = -1;
static char server_buf[SERVER_BUFFER_SIZE];
static char server_stack[THREAD_STACKSIZE_DEFAULT];
static msg_t server_msg_queue[SERVER_MSG_QUEUE_SIZE];

static void *_server_thread(void *args)
{
    struct sockaddr_in6 server_addr;
    uint16_t port;

    // TODO: Could we add an ifdef to include only in RIOT?
    msg_init_queue(server_msg_queue, SERVER_MSG_QUEUE_SIZE);

    // Create IPv6 TCP socket
    server_sock = socket(AF_INET6, SOCK_STREAM, 0);

    // parse port
    port = atoi((char *)args);
    if (port == 0) {
        puts("Error: invalid port specified");
        return NULL;
    }

    server_addr.sin6_family = AF_INET6;
    // Clear memory and set port
    memset(&server_addr.sin6_addr, 0, sizeof(server_addr.sin6_addr));
    server_addr.sin6_port = htons(port);

    // Check if socket type is supported
    if (server_sock < 0) {
        puts("Error initializing socket");
        server_sock = 0;
        return NULL;
    }

    // Bind socket to address
    if (bind(server_sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        server_sock = -1;
        puts("Error binding socket");
        return NULL;
    }

    // Start listening
    if (listen(server_sock, 4) < 0) {
        server_sock = -1;
        puts("Error listening in socket");
        return NULL;
    }

    printf("Success: started TCP server on port %" PRIu16 "\n", port);

    while (1) {
        int sock, res;
        struct sockaddr_in6 src;
        socklen_t src_len = sizeof(struct sockaddr_in6);

        char client_addr[IPV6_ADDR_MAX_STR_LEN];
        uint16_t client_port;

        if ((sock = accept(server_sock, (struct sockaddr *)&src, &src_len) < 0)) {
            puts("Error during accept");
            continue;
        }

        // Convert ipv6 addr to string representation
        inet_ntop(AF_INET6, &src.sin6_addr, client_addr, sizeof(client_addr));
        client_port = src.sin6_port;

        printf("TCP Client [%s]: %u connected\n", client_addr, client_port);

        while ((res = recv(server_sock, server_buf, sizeof(server_buf), 0)) >= 0) {
            printf("Received TCP data from client [%s]:%u:\n",
                   client_addr, client_port);

            if (res > 0) {
                printf("Received: \"");
                for (int i = 0; i < res; i++) {
                    printf("%c", server_buf[i]);
                }
                puts("\"");

                // Write data back
                if (write(sock, &server_buf, res) < 0) {
                    puts("Errored on write, finished server loop");
                    break;
                }
            }
            else {
                puts("Disconnected");
                break;
            }
        }
        printf("TCP connection to [%s]:%u reset, starting to accept again\n",
               client_addr, client_port);
        close(sock);
    }
    return NULL;
}

static int echo_send(char *addr_str, char *port_str, char *msg)
{
    struct sockaddr_in6 src, dst;
    uint16_t port;

    int sock;

    src.sin6_family = AF_INET6;
    dst.sin6_family = AF_INET6;

    int res;
    char buf[64];

    memset(&src.sin6_addr, 0, sizeof(src.sin6_addr));

    // parse destination address
    if (inet_pton(AF_INET6, addr_str, &dst.sin6_addr) != 1) {
        puts("Unable to parse destination address");
        return 1;
    }

    // parse port
    port = atoi(port_str);
    if (port == 0) {
        puts("Error: invalid port specified");
        return 1;
    }
    dst.sin6_port = htons(port);

    if ((sock = socket(AF_INET6, SOCK_STREAM, 0)) < 0) {
        perror("Error initializing socket");
        return 1;
    }

    if (connect(sock, (struct sockaddr *)&dst, sizeof(dst)) < 0) {
        perror("Error in connect");
        #if ENABLE_DEBUG
        /*
         * this switch is unneeded since perror dumps the string for errno
         * but it is useful for debugging
         */
        printf("errno is ");
        switch (errno) {
            case EACCES:
                puts("EACCESS");
                break;
            case EPERM:
                puts("EPERM");
                break;
            case EADDRINUSE:
                puts("EADDRINUSE");
                break;
            case EAFNOSUPPORT:
                puts("EAFNOSUPPORT");
                break;
            case EAGAIN:
                puts("EAGAIN");
                break;
            case EALREADY:
                puts("EALREADY");
                break;
            case EBADF:
                puts("EBADF");
                break;
            case ECONNREFUSED:
                puts("ECONNREFUSED");
                break;
            case EFAULT:
                puts("EFAULT");
                break;
            case EINPROGRESS:
                puts("EINPROGRESS");
                break;
            case EINTR:
                puts("EINTR");
                break;
            case EISCONN:
                puts("EISCONN");
                break;
            case ENETUNREACH:
                puts("ENETUNREACH");
                break;
            case ENOTSOCK:
                puts("ENOTSOCK");
                break;
            case ETIMEDOUT:
                puts("ETIMEDOUT");
                break;
            default:
                printf("unknown error: %d", errno);
                puts("");
                break;
        }
        #endif
        return 1;
    }

    // Send msg
    if (send(sock, msg, strlen(msg), 0) < 0) {
        puts("Error sending message");
        return 1;
    }

    // Receive reply
    if ((res = recv(sock, buf, sizeof(buf), 0)) < 0) {
        puts("Error receiving message");
        return 1;
    }

    // Print reply message
    for (int i = 0; i < res; i++) {
        printf("%c", buf[i]);
    }
    puts("");

    close(sock);
    return 0;
}

static int echo_start_server(char *port_str)
{
    /* check if server is already running */
    if (server_sock >= 0) {
        puts("Error: server already running");
        return 1;
    }
    /* start server (which means registering pktdump for the chosen port) */
    if (thread_create(server_stack, sizeof(server_stack), THREAD_PRIORITY_MAIN - 1,
                      THREAD_CREATE_STACKTEST,
                      _server_thread, port_str, "Echo server") <= KERNEL_PID_UNDEF) {
        server_sock = -1;
        puts("error initializing thread");
        return 1;
    }
    return 0;
}

int echo_cmd(int argc, char **argv)
{
    if (argc < 2) {
        printf("usage: %s [send|server]\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "server") == 0) {
        if (argc < 3) {
            printf("usage: %s server [start|stop]\n", argv[0]);
            return 1;
        }
        if (strcmp(argv[2], "start") == 0) {
            if (argc < 4) {
                printf("usage %s server start <port>\n", argv[0]);
                return 1;
            }
            return echo_start_server(argv[3]);
        }
        else {
            puts("error: invalid command");
            return 1;
        }
    }
    else if (strcmp(argv[1], "send") == 0) {
        if (argc < 5) {
            printf("usage: %s send <addr> <port> <msg>]\n",
                   argv[0]);
            return 1;
        }

        return echo_send(argv[2], argv[3], argv[4]);
    }
    else {
        puts("error: invalid command");
        return 1;
    }
    return 0;
}
