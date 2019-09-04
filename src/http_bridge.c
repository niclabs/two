/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http_bridge.h"
#include "http.h"
#include "http2.h"
#include "logging.h"


int http_write(hstates_t *hs, uint8_t *buf, int len)
{
    int wr = 0;

    if (hs->socket_state && hs->connection_state) {
        wr = sock_write(&hs->socket, (char *)buf, len);
    }
    else {
        ERROR("No client connected found");
        return -1;
    }

    if (wr <= 0) {
        ERROR("Error in writing");
        if (wr == 0) {
            hs->socket_state = 0;
        }
        return wr;
    }
    return wr;
}


int http_read(hstates_t *hs, uint8_t *buf, int len)
{
    int rd = 0;

    if (hs->socket_state && hs->connection_state) {
        rd = sock_read(&hs->socket, (char *)buf, len, 0);
    }
    else {
        ERROR("No client connected found");
        return -1;
    }

    INFO("http_read: read %d bytes from socket\n", rd);

    if (rd <= 0) {
        if (rd == 0) {
            hs->socket_state = 0;
        } else if (rd < 0) {
            ERROR("Error in reading: %d", rd);
        }
        return rd;
    }
    return rd;

}
