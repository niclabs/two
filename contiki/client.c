#include <stdio.h>

#include "contiki-net.h"
#include "sys/timer.h"
#include "sock.h"
#include "logging.h"

PROCESS(test_client_process, "Test client process");
AUTOSTART_PROCESSES(&test_client_process);

PROCESS_THREAD(test_client_process, ev, data){
    PROCESS_BEGIN();

    static sock_t client;
    while (1) {
        if (sock_create(&client) < 0) {
            FATAL("Could not create socket");
        }

        if (sock_connect(&client, "2001:dead:beef::1", 8888) < 0) {
            FATAL("Error connecting to server");
        }

        INFO("Waiting for connection ...");
        PROCESS_SOCK_WAIT_CONNECTION(&client);
        INFO("Connected to server");

        static char buf[64];
        static int bytes;
        sock_write(&client, "This is contiki\n", 16);
        while (1) {
            PROCESS_SOCK_WAIT_DATA(&client);  // wait reply
            if ((bytes = sock_read(&client, buf, sizeof(buf), 0)) <= 0) {
                ERROR("Could not read data");
                break;
            }

            if (bytes > 0) {
                INFO("Received data '%.*s'", bytes, buf);
                sock_write(&client, buf, bytes);
            }
        }
        sock_destroy(&client);
    }

    PROCESS_END();
}
