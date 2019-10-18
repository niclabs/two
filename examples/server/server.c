#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include "dos.h"
#include "logging.h"


void cleanup(int signal)
{
    (void)signal;

    PRINTF("Ctrl-C received");
}

int send_text(char * method, char * uri, uint8_t * response, int maxlen)
{
    (void)method;
    (void)uri;
    if (maxlen < 16){
      memcpy(response, "", 0);
      return 0;
    }
    memcpy(response, "hello world!!!!", 16);
    return 16;
}

int main(int argc, char **argv)
{
    if (argc < 2) {
        PRINTF("Usage: %s <port>\n", argv[0]);
        return 1;
    }

    int port = atoi(argv[1]);
    if (port < 0) {
        PRINTF("Invalid port given");
        return 1;
    }

    else {
        int rc = two_register_resource("GET", "/index", &send_text);
        if (rc < 0) {
            ERROR("in two_register_resource");
        }
        else {

            rc = two_server_start(port);
            if (rc < 0) {
                ERROR("in start server");
            }
        }
    }

}
