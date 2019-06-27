#include <stdio.h>

#include "contiki.h"

#include "sock.h"
#include "logging.h"

PROCESS(test_server_process, "Test server process");
AUTOSTART_PROCESSES(&test_server_process);

PROCESS_THREAD(test_server_process, ev, data){
    PROCESS_BEGIN();

    static sock_t server, client; 
    if (sock_create(&server) < 0) {
        FATAL("Could not create socket");
    }

    if (sock_listen(&server, 8888) < 0) {
        FATAL("Could not perform listen");
    }

    INFO("Listening on port 8888");

    while (1) {
        INFO("Waiting for client ...");
        PROCESS_SOCK_WAIT_CONNECTION(&server); 
        if (sock_accept(&server, &client) < 0) {
            ERROR("Error in accept");
            break;
        }
        INFO("New client connection");

        while (1) {
            char buf[8];
            int bytes = 0;
            PROCESS_SOCK_WAIT_DATA(&client); 
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
    sock_destroy(&server);

    PROCESS_END();
}
