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

    if (hs->socket_state == 1) {
        rd = sock_read(&hs->socket, (char *)buf, len, 0);
    }
    else {
        ERROR("No client connected found");
        return -1;
    }

    if (rd <= 0) {
        ERROR("Error in reading: %d", rd);
        if (rd == 0) {
            hs->socket_state = 0;
        }
        return rd;
    }
    return rd;

}


int http_clear_header_list(hstates_t *hs, int index, int flag)
{
    if (flag == 0) {
        if (index >= hs->hd_lists.headers_in.count && index <= HTTP2_MAX_HEADER_COUNT) {
            return 0;
        }
        if (index <= HTTP2_MAX_HEADER_COUNT && index >= 0) {
            hs->hd_lists.headers_in.count = index;
            return 0;
        }

        hs->hd_lists.headers_in.count = 0;

        return 0;
    }
    if (index >= hs->hd_lists.header_list_count_out && index <= HTTP2_MAX_HEADER_COUNT) {
        return 0;
    }
    if (index <= HTTP2_MAX_HEADER_COUNT && index >= 0) {
        hs->hd_lists.header_list_count_out = index;
        return 0;
    }

    hs->hd_lists.header_list_count_out = 0;

    return 0;

}
