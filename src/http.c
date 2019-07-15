/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>

#include "http.h"
#include "http_bridge.h"
#include "logging.h"
#include "sock.h"
#include "http2.h"

char * http_get_header(headers_data_lists_t *hd_lists, char *header, int header_size);
int http_set_data(headers_data_lists_t *hd_lists, uint8_t *data, int data_size);

int http_find_cb_for_resource(hstates_t * hs, char * path, callback_type_t * callback) {
    if (hs->path_callback_list_count == 0) {
        ERROR("Path-callback list is empty");
        return -1;
    }

    int i;
    for (i = 0; i <= hs->path_callback_list_count; i++) {
        char *path_in_list = hs->path_callback_list[i].name;
        if ((strncmp(path_in_list, path, strlen(path)) == 0) && strlen(path) == strlen(path_in_list)) {
            callback->cb = hs->path_callback_list[i].ptr;
            break;
        }
        if (i == hs->path_callback_list_count) {
            ERROR("No callback associated for the given path");
            return -1;
        }
    }

    return 0;
}

void http_states_init(hstates_t *hs)
{
    memset(hs, 0, sizeof(*hs));
}

int http_error(hstates_t * hs, int code, char * msg) {
    // Set status code
    char strCode[4];
    snprintf(strCode, 4, "%d", code);
    http_set_header(&hs->hd_lists, ":status", strCode);

    // Set error message
    if (msg != NULL) {
        http_set_data(&hs->hd_lists, (uint8_t *)msg, strlen(msg));
    }

    // Send response
    if (h2_send_response(hs) < 0) {
        ERROR("Could not send data");
        return -1;
    }
    return 0;
}


int http_recv_headers(hstates_t *hs) {
    while(hs->connection_state == 1) {
        // receive frame
        if (h2_receive_frame(hs) < 0) {
            break;
        }

        // if we need to wait for continuation
        if (hs->keep_receiving == 1) {
            continue;
        }

        // when new headers are available return
        if (hs->new_headers == 1) {
            return 0;
        }
    }
    return -1;
}

int http_process_request(hstates_t *hs) {
    // Get uri
    char * uri = http_get_header(&hs->hd_lists, ":path", 5);

    // TODO: parse URI removing query parameters

    // find callback for resource
    callback_type_t callback;
    if (http_find_cb_for_resource(hs, uri, &callback) < 0) {
        // 404
        return http_error(hs, 404, "Not Found");
    }

    // Set default headers
    http_set_header(&hs->hd_lists, ":status", "200");

    // Get response from callback
    callback.cb(&hs->hd_lists);

    // Send response
    if (h2_send_response(hs) < 0) {
        ERROR("Problems sending data");
        return -1;
    }

    return 0;
}
/************************************Server************************************/
int http_server_create(hstates_t *hs, uint16_t port)
{
    http_states_init(hs);

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

int http_server_start(hstates_t *hs)
{
    INFO("Server waiting for a client\n");


    while (1) {
        if (sock_accept(&hs->server_socket, &hs->socket) < 0) {
            break;
        }
        DEBUG("New client connection");

        // Update http_state
        hs->socket_state = 1;
        hs->connection_state = 1;

        // Initialize http2 connection (send headers)
        if (h2_server_init_connection(hs) < 0) {
            ERROR("Could not perform HTTP/2 initialization");

            // TODO: terminate client and continue
            //sock_destroy(&hs->socket);
            hs->connection_state = 0;
            //hs->socket_state = 0;
            return -1;
        }

        while (hs->connection_state == 1) {
            // read headers
            hs->new_headers = 0;
            if (http_recv_headers(hs) < 0) {
                ERROR("An error ocurred while receiving headers");
                break;
            }

            // Get the method from headers
            char * method = http_get_header(&hs->hd_lists, ":method", 7);
            if (strncmp(method, "GET", 3) != 0) {
                http_error(hs, 501, "Not Implemented");

                // TODO: what else to do here?
                http_clear_header_list(hs, -1, 0);
                continue;
            }

            // TODO: read data (if POST)

            // Clear headers (why?)
            http_clear_header_list(hs, -1, 1);

            // Process the http request
            http_process_request(hs);
           
            // Clear headers (why?)
            http_clear_header_list(hs, -1, 0);
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

    INFO("Server destroyed\n");

    return 0;
}


int http_set_resource_cb(hstates_t *hs, callback_type_t callback, char *path)
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
    http_states_init(hs);

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

    INFO("Client connected to server\n");

    hs->connection_state = 1;

    if (h2_client_init_connection(hs) < 0) {
        ERROR("Problems sending client data");
        return -1;
    }

    return 0;
}

int http_start_client(hstates_t *hs)
{
    http_clear_header_list(hs, -1, 0);
    while (hs->connection_state == 1) {
        if (h2_receive_frame(hs) < 0) {
            break;
        }
        if (hs->keep_receiving == 1) {
            continue;
        }
        if (hs->hd_lists.data_in_size > 0) {
          return 0;
        }
    }
    INFO("Client off the while");
    hs->connection_state = 0;
    hs->socket_state = 0;
    if (sock_destroy(&hs->socket) == -1) {
        WARN("Could not destroy client socket");
        return -1;
    }
    return -1;
}



int http_get(hstates_t *hs, char *path, char *host, char *accept_type, response_received_type_t *rr)
{
    int method = http_set_header(&hs->hd_lists, ":method", "GET");
    int scheme = http_set_header(&hs->hd_lists, ":scheme", "https");
    int set_path = http_set_header(&hs->hd_lists, ":path", path);
    int set_host = http_set_header(&hs->hd_lists, "host", host);
    int set_accept = http_set_header(&hs->hd_lists, "accept", accept_type);

    if (method < 0 || scheme < 0 || set_path < 0 || set_host < 0 || set_accept < 0) {
        ERROR("Cannot add headers to query");
        return -1;
    }
    if (h2_send_request(hs) < 0) {
        ERROR("Cannot send query");
        return -1;
    }
    http_clear_header_list(hs, -1, 1);

    if (http_start_client(hs) < 0) {
        rr->status_flag=400;
        rr->size_data = 0;
        return -1;
    }

    if (hs->hd_lists.data_in_size > 0) {
        rr->size_data = http_get_data(&hs->hd_lists, rr->data);
        rr->status_flag = atoi(http_get_header(&hs->hd_lists, ":status", 7));
    }else{
        rr->size_data = 0;
    }
    return 0;
}



int http_client_disconnect(hstates_t *hs)
{
    if (hs->socket_state == 1) {
        if (sock_destroy(&hs->socket) < 0) {
            ERROR("Error in client disconnection");
            return -1;
        }

        INFO("Client disconnected\n");
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


char *http_get_header(headers_data_lists_t *hd_lists, char *header, int header_size)
{
    int i = hd_lists->header_list_count_in;

    if (i == 0) {
        WARN("Headers list is empty");
        return NULL;
    }

    int k;
    size_t header_size_t = header_size;
    for (k = 0; k < i; k++) {
        if ((strncmp(hd_lists->header_list_in[k].name, header, header_size) == 0) && header_size_t == strlen(hd_lists->header_list_in[k].name)) {
            INFO("RETURNING value of '%s' header; '%s'", hd_lists->header_list_in[k].name, hd_lists->header_list_in[k].value);
            return hd_lists->header_list_in[k].value;
        }
    }

    WARN("Header '%s' not found in headers list", header);
    return NULL;
}


uint32_t http_get_data(headers_data_lists_t *hd_lists, uint8_t *data_buffer)
{
    if (hd_lists->data_in_size == 0) {
        WARN("Data list is empty");
        return 0;
    }
    memcpy(data_buffer, hd_lists->data_in, hd_lists->data_in_size);
    return hd_lists->data_in_size;
}


int http_set_data(headers_data_lists_t *hd_lists, uint8_t *data, int data_size)
{
    if (HTTP_MAX_DATA_SIZE == hd_lists->data_out_size) {
        ERROR("Data buffer full");
        return -1;
    }
    if (data_size <= 0 || data_size > 128) {
        ERROR("Data too large for buffer size");
        return -1;
    }
    hd_lists->data_out_size = data_size;
    memcpy(hd_lists->data_out, data, data_size);
    return 0;
}



