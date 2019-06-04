/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http_methods_bridge.h"
#include "http_methods.h"
#include "logging.h"


int http_write(uint8_t *buf, int len, hstates_t *hs)
{
    int wr = 0;

    if (hs->socket_state == 1) {
        wr = sock_write(hs->socket, (char *)buf, len);
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


int http_read( uint8_t *buf, int len, hstates_t *hs)
{
    int rd = 0;

    if (hs->socket_state == 1) {
        rd = sock_read(hs->socket, (char *)buf, len, 0);
    }
    else {
        ERROR("No client connected found");
        return -1;
    }

    if (rd <= 0) {
        ERROR("Error in reading");
        if (rd == 0) {
            hs->socket_state = 0;
        }
        return rd;
    }
    return rd;

}

int http_receive(char *headers)
{
    (void)headers;
    // TODO: read headers
    // TODO: identify HTTP method
    // TODO: call get_receive
    return -1;
}


int get_receive(char *path, char *headers)
{
    (void)path;
    (void)headers;
    // preparar respuesta
    // buscar callback correspondiente a path
    // llamar a callback y agregar datos a respuesta
    // codificar respuesta usando http2
    return -1;
}


int http_clear_header_list(hstates_t *hs, int index)
{
    if (index>=hs->table_count){
      return 0;
    }
    if (index <= HTTP2_MAX_HEADER_COUNT && index >= 0) {
        hs->table_count = index;
        return 0;
    }

    hs->table_count = 0;

    return 0;
}
