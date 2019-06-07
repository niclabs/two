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


int http_read(hstates_t *hs, uint8_t *buf, int len)
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

int http_receive(hstates_t *hs)
{
    (void)hs;
    // TODO: read headers
    // TODO: identify HTTP method
    // TODO: call get_receive
    return -1;
}


int get_receive(hstates_t *hs, char *path)
{
    // preparar respuesta

    callback_type_t callback;

    if (hs->path_callback_list_count == 0) {
        WARN("Path-callback list is empty");
        return -1;
    }

    int i;
    for (i = 0; i <= hs->path_callback_list_count; i++) {
        if (strncmp(hs->path_callback_list[i].name, path, strlen(path)) == 0) {
            callback.cb = hs->path_callback_list[i].ptr;
            break;
        }
        if (i == hs->path_callback_list_count) {
            WARN("No function associated with this path");
            return -1;
        }
    }

    callback.cb(&hs->h_lists);

    if(h2_send_headers(hs)<0){
      return -1;
    }

    return 0;
}


int http_clear_header_list(hstates_t *hs, int index)
{
    if (index>=hs->h_lists.header_list_count){
      return 0;
    }
    if (index <= HTTP2_MAX_HEADER_COUNT && index >= 0) {
        hs->h_lists.header_list_count = index;
        return 0;
    }

    hs->h_lists.header_list_count = 0;

    return 0;
}
