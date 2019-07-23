/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#include "headers.h"
#include "http.h"
#include "http_bridge.h"
#include "logging.h"
#include "sock.h"
#include "http2.h"


#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif

/*********************************************************
* Private HTTP API methods
*********************************************************/

/**
 * Parse URI into path and query parameters
 *
 * TODO: This function should probably be an API function
 * TODO: improve according to https://tools.ietf.org/html/rfc3986
 */
int parse_uri(char *uri, char *path, char *query_params)
{
    if (uri == NULL || path == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (strlen(uri) == 0) {
        strcpy(path, "/");
    }

    char *ptr = index(uri, '?');
    if (ptr) {
        if (query_params) {
            strcpy(query_params, ptr + 1);
        }
        *ptr = '\0';
    }
    else if (query_params) {
        strcpy(query_params, "");
    }

    strcpy(path, uri);
    return 0;
}

/*
 * Add a header and its value to the headers list
 *
 * @param    hd_lists         Struct with headers information
 * @param    name             New headers name
 * @param    value            New headers value
 *
 * @return   0                Successfully added pair
 * @return   -1               There was an error in the process
 */
int http_set_header(headers_data_lists_t *hd_lists, char *name, char *value)
{
    int i = hd_lists->headers_out.count;

    if (i == HTTP2_MAX_HEADER_COUNT) {
        WARN("Headers list is full");
        return -1;
    }

    strcpy(hd_lists->headers_out.headers[i].name, name);
    strcpy(hd_lists->headers_out.headers[i].value, value);

    hd_lists->headers_out.count = i + 1;

    return 0;
}

/*
 * Search by a value of a header in the header list
 *
 * @param    hd_lists         Struct with headers information
 * @param    header           Header name
 * @param    header_size      Size of header name
 *
 * @return                    Value finded
 */
char *http_get_header(headers_data_lists_t *hd_lists, char *header, int header_size)
{
    int i = hd_lists->headers_in.count;

    if (i == 0) {
        WARN("Headers list is empty");
        return NULL;
    }

    int k;
    size_t header_size_t = header_size;
    for (k = 0; k < i; k++) {
        if ((strncmp(hd_lists->headers_in.headers[k].name, header, header_size) == 0) && header_size_t == strlen(hd_lists->headers_in.headers[k].name)) {
            DEBUG("RETURNING value of '%s' header; '%s'", hd_lists->headers_in.headers[k].name, hd_lists->headers_in.headers[k].value);
            return hd_lists->headers_in.headers[k].value;
        }
    }

    WARN("Header '%s' not found in headers list", header);
    return NULL;
}

/**
 * Utility function to check for method support
 *
 * @returns 1 if the method is supported by the implementation, 0 if not
 */
int has_method_support(char *method)
{
    if (strncmp("GET", method, 8) != 0) {
        return 0;
    }
    return 1;
}

/**
 * Get received data
 *
 * @param    hd_lists         Struct with data information
 * @param    data_buffer      Buffer for data received
 *
 * @return                    Data lengt
 */
uint32_t get_data(headers_data_lists_t *hd_lists, uint8_t *data_buffer, size_t size)
{
    int copysize = MIN(hd_lists->data_in_size, size);

    memcpy(data_buffer, hd_lists->data_in, copysize);
    return copysize;
}

/*
 * Add data to be sent to data lists
 *
 * @param    hd_lists   Struct with data information
 * @param    data       Data
 * @param    data_size  Size of data
 *
 * @return   0          Successfully added data
 * @return   -1         There was an error in the process
 */
int set_data(headers_data_lists_t *hd_lists, uint8_t *data, int data_size)
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

/*
 * Check for valid HTTP path according to
 * RFC 2396 (see https://tools.ietf.org/html/rfc2396#section-3.3)
 *
 * TODO: for now this function only checks that the path starts
 * by a '/'. Validity of the path should be implemented according to
 * the RFC
 *
 * @return 1 if the path is valid or 0 if not
 * */
int is_valid_path(char *path)
{
    if (path[0] != '/') {
        return 0;
    }
    return 1;
}

/**
 * Get a resource handler for the given path
 */
http_resource_handler_t get_resource_handler(hstates_t *hs, char *method, char *path)
{
    http_resource_t res;

    for (int i = 0; i < hs->resource_list_size; i++) {
        res = hs->resource_list[i];
        if (strncmp(res.path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res.method, method) == 0) {
            return res.handler;
        }
    }
    return NULL;
}

/**
 * Set all hstates values to its initial values
 */
void reset_http_states(hstates_t *hs)
{
    memset(hs, 0, sizeof(*hs));
    headers_t headers_in;
    int maxlen = 20; //TODO static for now...
    header_t hlist_in[maxlen];
    headers_init(&headers_in, hlist_in, maxlen);

    headers_t headers_out;
    header_t hlist_out[maxlen];
    headers_init(&headers_out, hlist_out, maxlen);
    hs->hd_lists.headers_in = headers_in;
    hs->hd_lists.headers_out = headers_out;
}

/**
 * Send an http error with the given code and message
 */
int error(hstates_t *hs, int code, char *msg)
{
    // Set status code
    char strCode[4];

    snprintf(strCode, 4, "%d", code);
    http_set_header(&hs->hd_lists, ":status", strCode);

    // Set error message
    if (msg != NULL) {
        set_data(&hs->hd_lists, (uint8_t *)msg, strlen(msg));
    }

    // Send response
    if (h2_send_response(hs) < 0) {
        ERROR("Could not send data");
        return -1;
    }
    return 0;
}

/**
 * Read headers from the request
 */
int receive_headers(hstates_t *hs)
{
    while (hs->connection_state == 1) {
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

/**
 * Perform request for the given method and uri
 */
int do_request(hstates_t *hs, char *method, char *uri)
{
    // parse URI removing query parameters
    char path[HTTP_MAX_PATH_SIZE];

    parse_uri(uri, path, NULL);

    // find callback for resource
    http_resource_handler_t handle_uri;
    if ((handle_uri = get_resource_handler(hs, method, path)) == NULL) {
        // 404
        return error(hs, 404, "Not Found");
    }

    // Set default headers
    http_set_header(&hs->hd_lists, ":status", "200");

    // Prepare response for callback
    // TODO: response pointer should be pointer to hs->data_out
    uint8_t response[HTTP_MAX_RESPONSE_SIZE];
    int len;
    if ((len = handle_uri(method, uri, response, HTTP_MAX_RESPONSE_SIZE)) < 0) {
        // if the handler returns
        return error(hs, 500, "Server Error");
    }
    else if (len > 0) {
        set_data(&hs->hd_lists, response, len);
    }

    // Send response
    if (h2_send_response(hs) < 0) {
        // TODO: get error code from HTTP/2
        ERROR("HTTP/2 error ocurred. Could not send data");
        return -1;
    }

    return 0;
}

/************************************
* Server API methods
************************************/

int http_server_create(hstates_t *hs, uint16_t port)
{
    reset_http_states(hs);

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
            if (receive_headers(hs) < 0) {
                ERROR("An error ocurred while receiving headers");
                break;
            }

            // Get the method from headers
            char *method = http_get_header(&hs->hd_lists, ":method", 7);
            DEBUG("Received %s request", method);
            if (!has_method_support(method)) {
                error(hs, 501, "Not Implemented");

                // TODO: what else to do here?
                http_clear_header_list(hs, -1, 0);
                continue;
            }

            // TODO: read data (if POST)

            // Clear headers (why?)
            http_clear_header_list(hs, -1, 1);

            // Get uri
            char *uri = http_get_header(&hs->hd_lists, ":path", 5);

            // Process the http request
            do_request(hs, method, uri);

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

int http_server_register_resource(hstates_t *hs, char *method, char *path, http_resource_handler_t handler)
{
    if (hs == NULL || method == NULL || path == NULL || handler == NULL) {
        errno = EINVAL;
        return -1;
    }

    if (!has_method_support(method)) {
        errno = EINVAL;
        ERROR("Method %s not implemented yet", method);
        return -1;
    }

    if (!is_valid_path(path)) {
        errno = EINVAL;
        ERROR("Path %s does not have a valid format", path);
        return -1;
    }

    if (strlen(path) >= HTTP_MAX_PATH_SIZE) {
        errno = EINVAL;
        ERROR("Path length is larger than max supported size (%d). Try updating HTTP_CONF_MAX_PATH_SIZE", HTTP_MAX_PATH_SIZE);
        return -1;
    }

    http_resource_t *res;
    for (int i = 0; i < hs->resource_list_size; i++) {
        res = &hs->resource_list[i];
        if (strncmp(res->path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res->method, method) == 0) {
            res->handler = handler;
            return 0;
        }
    }

    if (hs->resource_list_size >= HTTP_MAX_RESOURCES) {
        ERROR("HTTP resource limit (%d) reached. Try changing value for HTTP_CONF_MAX_RESOURCES", HTTP_MAX_RESOURCES);
        return -1;
    }

    res = &hs->resource_list[hs->resource_list_size++];

    // Set values
    strncpy(res->method, method, 8);
    strncpy(res->path, path, HTTP_MAX_PATH_SIZE);
    res->handler = handler;

    return 0;
}

/************************************
* Client API methods
************************************/

int receive_server_response(hstates_t *hs)
{
    // TODO: receive headers (?)

    http_clear_header_list(hs, -1, 0); // why?

    // Receive data
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

    return -1;
}

int send_client_request(hstates_t *hs, char *method, char *uri, uint8_t *response, size_t *size)
{
    if (http_set_header(&hs->hd_lists, ":method", method) < 0 ||
        http_set_header(&hs->hd_lists, ":scheme", "http") < 0 ||
        http_set_header(&hs->hd_lists, ":path", uri) < 0 ||
        http_set_header(&hs->hd_lists, "Host", hs->host) < 0) {
        ERROR("Failed to set headers for request");
        return -1;
    }

    // Try to send request
    if (h2_send_request(hs) < 0) {
        ERROR("Failed to send request %s %s", method, uri);
        return -1;
    }

    if (receive_server_response(hs) < 0) {
        ERROR("An error ocurred while waiting for server response");
        return -1;
    }

    int status = atoi(http_get_header(&hs->hd_lists, ":status", 7));

    // Get response data (TODO: should we just copy the pointer?)
    *size = get_data(&hs->hd_lists, response, *size);

    return status;
}

int http_client_connect(hstates_t *hs, char *addr, uint16_t port)
{
    reset_http_states(hs);

    hs->is_server = 0;

    if (sock_create(&(hs->socket)) < 0) {
        ERROR("Failed to create socket for client connection");
        return -1;
    }

    hs->socket_state = 1;

    if (sock_connect(&hs->socket, addr, port) < 0) {
        ERROR("Connection to remote address %s failed", addr);
        http_client_disconnect(hs);
        return -1;
    }

    INFO("Connected to %s", addr);

    // Connected
    hs->connection_state = 1;

    // Initialize http/2 connection
    if (h2_client_init_connection(hs) < 0) {
        ERROR("Failed to perform HTTP/2 initialization");
        return -1;
    }

    // Copy address into host field
    strncpy(hs->host, addr, HTTP_MAX_HOST_SIZE);

    return 0;
}

int http_get(hstates_t *hs, char *uri, uint8_t *response, size_t *size)
{
    return send_client_request(hs, "GET", uri, response, size);
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
