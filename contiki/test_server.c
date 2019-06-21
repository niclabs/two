#include <stdio.h>

#include "contiki-net.h"
#include "sys/timer.h"
#include "sock.h"
#include "logging.h"



PROCESS(test_sock_process, "Test sock process");
AUTOSTART_PROCESSES(&test_sock_process);

PROCESS_THREAD(test_sock_process, ev, data){
    PROCESS_BEGIN();

    static sock_t server, client; 
	//static struct timer timer;
    if (sock_create(&server) < 0) {
        FATAL("Could not create socket");
    }

    if (sock_listen(&server, 8888) < 0) {
        FATAL("Could not perform listen");
    }

    INFO("Listening on port 8888");

    while (1) {
        PROCESS_SOCK_WAIT_CLIENT(&server); 
        if (sock_accept(&server, &client) < 0) {
            ERROR("Error in accept");
            continue;
        }
        INFO("New client connection");

        while (1) {
            char buf[8];
            int bytes = 0;
            PROCESS_SOCK_WAIT_DATA(&client); 
            if ((bytes = sock_read(&client, buf, sizeof(buf), 0)) < 0) {
                ERROR("Could not read data");
                continue;
            } 
			
			if (bytes > 0) {
            	INFO("Received data '%.*s'", bytes, buf);
                sock_write(&client, buf, bytes);
			}

			//timer_set(&timer, CLOCK_SECOND * 5); // wait 5 seconds
        	//PROCESS_WAIT_UNTIL(timer_expired(&timer));
        }

    }

    sock_destroy(&server);

    PROCESS_END();
}
