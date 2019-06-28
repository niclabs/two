/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>

#include <unistd.h>

#include "http.h"
#include "http_bridge.h"
#include "logging.h"
#include "sock.h"
#include "http2.h"

int get_receive(hstates_t *hs);

void set_init_values(hstates_t *hs)
{
    hs->socket_state = 0;
    hs->h_lists.header_list_count_in = 0;
    hs->h_lists.header_list_count_out = 0;
    hs->path_callback_list_count = 0;
    hs->connection_state = 0;
    hs->server_socket_state = 0;
    hs->keep_receiving = 0;
    hs->new_headers = 0;
    hs->data_in_size = 0;
    hs->data_out_size = 0;
}

/************************************Server************************************/


int http_init_server(hstates_t *hs, uint16_t port)
{
    set_init_values(hs);

    hs->is_server = 1;

    if (sock_create(&hs->server_socket) < 0) {
        ERROR("Error in server creation");
        return -1;
    }

    hs->server_socket_state = 1;

    if (sock_listen(&hs->server_socket, port) < 0) {
        ERROR("Partial error in server creation");
        if (sock_destroy(&hs->server_socket) == 0) {
            hs->server_socket_state = 0;
        }
        return -1;
    }

    return 0;
}


int http_start_server(hstates_t *hs)
{
    printf("Server waiting for a client\n");


    while (sock_accept(&hs->server_socket, &hs->socket) >= 0) {

        printf("Client found and connected\n");

        hs->socket_state = 1;
        hs->connection_state = 1;

        if (h2_server_init_connection(hs) < 0) {
            hs->connection_state = 0;
            ERROR("Problems sending server data");
            return -1;
        }

        while (hs->connection_state == 1) {
            if (h2_receive_frame(hs) < 0) {
                break;
            }
            if (hs->keep_receiving == 1) {
                continue;
            }
            if (hs->new_headers == 1) {
                http_clear_header_list(hs, -1, 1);
                get_receive(hs);
                http_clear_header_list(hs, -1, 0);
                hs->new_headers = 0;
            }
        }

        if (sock_destroy(&hs->socket) == -1) {
            WARN("Could not destroy client socket");
        }
        hs->connection_state = 0;
        hs->socket_state = 0;
    }

    ERROR("Not client found");

    return -1;
}


int http_server_destroy(hstates_t *hs)
{
    if (hs->server_socket_state == 0) {
        WARN("Server not found");
        return -1;
    }

    if (hs->socket_state == 1) {
        if (sock_destroy(&hs->socket) < 0) {
            WARN("Client still connected");
        }
    }

    hs->socket_state = 0;
    hs->connection_state = 0;

    if (sock_destroy(&hs->server_socket) < 0) {
        ERROR("Error in server disconnection");
        return -1;
    }

    hs->server_socket_state = 0;

    printf("Server destroyed\n");

    return 0;
}


int http_set_function_to_path(hstates_t *hs, callback_type_t callback, char *path)
{
    INFO("setting function to path '%s'", path);
    int i = hs->path_callback_list_count;

    if (i == HTTP_MAX_CALLBACK_LIST_ENTRY) {
        WARN("Path-callback list is full");
        return -1;
    }

    strcpy(hs->path_callback_list[i].name, path);
    hs->path_callback_list[i].ptr = callback.cb;

    hs->path_callback_list_count = i + 1;

    return 0;
}


/************************************Client************************************/


int http_client_connect(hstates_t *hs, uint16_t port, char *ip)
{
    set_init_values(hs);

    hs->is_server = 0;

    if (sock_create(&(hs->socket)) < 0) {
        ERROR("Error on client creation");
        return -1;
    }

    hs->socket_state = 1;

    if (sock_connect(&hs->socket, ip, port) < 0) {
        ERROR("Error on client connection");
        http_client_disconnect(hs);
        return -1;
    }

    printf("Client connected to server\n");

    hs->connection_state = 1;

    if (h2_client_init_connection(hs) < 0) {
        ERROR("Problems sending client data");
        return -1;
    }

    return 0;
}

int http_start_client(hstates_t *hs)
{
    while (hs->connection_state == 1) {
        if (h2_receive_frame(hs) < 0) {
            break;
        }
        if (hs->keep_receiving == 1) {
            continue;
        }
        if (hs->new_headers == 1) {
            get_receive(hs);
            http_clear_header_list(hs, -1, 0);
            http_clear_header_list(hs, -1, 1);
        }
    }
    hs->connection_state = 0;
    hs->socket_state = 0;
    if (sock_destroy(&hs->socket) == -1) {
        WARN("Could not destroy client socket");
        return -1;
    }
    return 0;
}


int http_get(hstates_t *hs, char *path, char *host, char *accept_type)
{
    int method = http_set_header(&hs->h_lists, ":method", "GET");
    int scheme = http_set_header(&hs->h_lists, ":scheme", "https");
    int set_path = http_set_header(&hs->h_lists, ":path", path);
    int set_host = http_set_header(&hs->h_lists, "host", host);
    int set_accept = http_set_header(&hs->h_lists, "accept", accept_type);

    if (method < 0 || scheme < 0 || set_path < 0 || set_host < 0 || set_accept < 0) {
        ERROR("Cannot add headers to query");
        return -1;
    }
    if (h2_send_request(hs) < 0) {
        ERROR("Cannot send query");
        return -1;
    }

    http_clear_header_list(hs, -1, 1);


    while (hs->connection_state == 1) {
        if (h2_receive_frame(hs) < 0) {
            break;
        }
        if (hs->keep_receiving == 1) {
            continue;
        }
        if (hs->new_headers == 1) {

            return 0;
        }
    }

    return -1;
}


int http_client_disconnect(hstates_t *hs)
{
    if (hs->socket_state == 1) {
        if (sock_destroy(&hs->socket) < 0) {
            ERROR("Error in client disconnection");
            return -1;
        }

        printf("Client disconnected\n");
    }

    hs->socket_state = 0;
    hs->connection_state = 0;

    return 0;
}


/************************************Headers************************************/

int http_set_header(headers_data_lists_t *hd_lists, char *name, char *value)
{
    int i = hd_lists->header_list_count_out;

    if (i == HTTP2_MAX_HEADER_COUNT) {
        WARN("Headers list is full");
        return -1;
    }

    strcpy(hd_lists->header_list_out[i].name, name);
    strcpy(hd_lists->header_list_out[i].value, value);

    hd_lists->header_list_count_out = i + 1;

    return 0;
}


char *http_get_header(headers_data_lists_t *hd_lists, char *header)
{
    int i = hd_lists->header_list_count_in;

    if (i == 0) {
        WARN("Headers list is empty");
        return NULL;
    }

    int k;
    for (k = 0; k < i; k++) {
        if ((strncmp(hd_lists->header_list_in[k].name, header, strlen(header)) == 0) && strlen(header) == strlen(hd_lists->header_list_in[k].name)) {
            INFO("RETURNING value of '%s' header; '%s'", hd_lists->header_list_in[k].name, hd_lists->header_list_in[k].value);
            return hd_lists->header_list_in[k].value;
        }
    }

    WARN("Header '%s' not found in headers list", header);
    return NULL;
}


uint8_t *http_get_data(headers_data_lists_t *hd_lists){
  return hd_lists->data_out;
}


int get_receive(hstates_t *hs)
{
    char *path = http_get_header(&hs->h_lists, ":path");
    callback_type_t callback;

    if (hs->path_callback_list_count == 0) {
        WARN("Path-callback list is empty");
        //return value 0 => an error can be send, 1 => problems
        return http_set_header(&hs->h_lists, ":status", "400");
    }

    int i;
    for (i = 0; i <= hs->path_callback_list_count; i++) {
        char *path_in_list = hs->path_callback_list[i].name;
        if ((strncmp(path_in_list, path, strlen(path)) == 0) && strlen(path) == strlen(path_in_list)) {
            callback.cb = hs->path_callback_list[i].ptr;
            break;
        }
        if (i == hs->path_callback_list_count) {
            WARN("No function associated with this path");
            //return value 0 => an error can be send, 1 => problems
            return http_set_header(&hs->h_lists, ":status", "400");
        }
    }

    http_set_header(&hs->h_lists, ":status", "200");
    callback.cb(&hs->h_lists);

    if (h2_send_response(hs) < 0) {
        ERROR("Problems sending data");
        return -1;
    }

    return 0;
}
