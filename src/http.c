/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)


#include "http.h"
#include "headers.h"
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

/**
 * Utility function to check for method support
 *
 * @returns 1 if the method is supported by the implementation, 0 if not
 */
int has_method_support(char *method)
{
    if ((strncmp("GET", method, 8) != 0) &&
    (strncmp("HEAD", method, 8) != 0)){
        return 0;
    }
    return 1;
}

/**
 * Get received data
 *
 * @param    data_in         Struct with data information
 * @param    data_buffer      Buffer for data received
 *
 * @return                    Data lengt
 */
uint32_t get_data(http_data_t *data_in, uint8_t *data_buffer, size_t size)
{
    int copysize = MIN(data_in->size, size);

    memcpy(data_buffer, data_in->buf, copysize);
    return copysize;
}

/*
 * Add data to be sent to data lists
 *
 * @param    data_out   Struct with data information
 * @param    data       Data
 * @param    data_size  Size of data
 *
 * @return   0          Successfully added data
 * @return   -1         There was an error in the process
 */
int set_data(http_data_t *data_out, uint8_t *data, int data_size)
{
    if (HTTP_MAX_DATA_SIZE == data_out->size) {
        ERROR("Data buffer full");
        return -1;
    }
    if (data_size <= 0 || data_size > 128) {
        ERROR("Data too large for buffer size");
        return -1;
    }
    data_out->size = data_size;
    memcpy(data_out->buf, data, data_size);
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
}

/**
 * Send an http error with the given code and message
 */
int error(hstates_t *hs, int code, char *msg)
{
    // Set status code
    char strCode[4];

    snprintf(strCode, 4, "%d", code);

    // Initialize header list
    header_t header_list[HTTP_MAX_HEADER_COUNT];
    headers_init(&hs->headers_out, header_list, HTTP_MAX_HEADER_COUNT);
    headers_set(&hs->headers_out, ":status", strCode);

    // Set error message
    if (msg != NULL) {
        set_data(&hs->data_out, (uint8_t *)msg, strlen(msg));
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
    hs->new_headers = 0;
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
        return error(hs, 404, "Not Found");
    }

    // If it is GET method Prepare response for callback
    if (strncmp("GET", method, 8) != 0){

        // TODO: response pointer should be pointer to hs->data_out
        uint8_t response[HTTP_MAX_RESPONSE_SIZE];
        int len;
        if ((len = handle_uri(method, uri, response, HTTP_MAX_RESPONSE_SIZE)) < 0) {
            // if the handler returns
            return error(hs, 500, "Server Error");
        }
        else if (len > 0) {
            set_data(&hs->data_out, response, len);
        }
    }

    // Initialize header list
    header_t header_list[HTTP_MAX_HEADER_COUNT];
    headers_init(&hs->headers_out, header_list, HTTP_MAX_HEADER_COUNT);

    // Set default headers
    headers_set(&hs->headers_out, ":status", "200");

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
    INFO("Server waiting for a client");

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
            sock_destroy(&hs->socket);
            hs->connection_state = 0;
            hs->socket_state = 0;
            return -1;
        }

        while (hs->connection_state == 1) {
            // Initialize input header list
            header_t header_list[HTTP_MAX_HEADER_COUNT];
            headers_init(&hs->headers_in, header_list, HTTP_MAX_HEADER_COUNT);

            // read headers
            if (receive_headers(hs) < 0) {
                ERROR("An error ocurred while receiving headers");
                break;
            }

            // Get the method from headers
            char * method = headers_get(&hs->headers_in, ":method");
            DEBUG("Received %s request", method);
            if (!has_method_support(method)) {
                error(hs, 501, "Not Implemented");

                // TODO: what else to do here?
                continue;
            }

            // TODO: read data (if POST)

            // Get uri
            char * uri = headers_get(&hs->headers_in, ":path");

            // Process the http request
            do_request(hs, method, uri);
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
        ERROR("ERROR found %d", errno );
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

    // Checks if the path and method already exist
    http_resource_t *res;
    for (int i = 0; i < hs->resource_list_size; i++) {
        res = &hs->resource_list[i];
        //If it does, replaces the resource
        if (strncmp(res->path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res->method, method) == 0) {
            res->handler = handler;
            return 0;
        }
    }

    // Checks if the list is full
    if (hs->resource_list_size >= HTTP_MAX_RESOURCES) {
        ERROR("HTTP resource limit (%d) reached. Try changing value for HTTP_CONF_MAX_RESOURCES", HTTP_MAX_RESOURCES);
        return -1;
    }

    // Adds the resource to the list
    res = &hs->resource_list[hs->resource_list_size++];

    // Sets values
    strncpy(res->method, method, 8);
    strncpy(res->path, path, HTTP_MAX_PATH_SIZE);
    res->handler = handler;

    return 0;
}


/************************************
* Client API methods
************************************/

int receive_server_response_data(hstates_t *hs)
{
    hs->end_message = 0;
    while (hs->connection_state == 1) {
        if (hs->end_message > 0) {
            return hs->end_message;
        }
        if (h2_receive_frame(hs) < 0) {
            break;
        }
    }

    return -1;
}

int send_client_request(hstates_t *hs, char *method, char *uri, uint8_t *response, size_t *size)
{
    // Initialize output header list
    header_t header_list_out[HTTP_MAX_HEADER_COUNT];

    headers_init(&hs->headers_out, header_list_out, HTTP_MAX_HEADER_COUNT);

    if (headers_set(&hs->headers_out, ":method", method) < 0 ||
        headers_set(&hs->headers_out, ":scheme", "http") < 0 ||
        headers_set(&hs->headers_out, ":path", uri) < 0 ||
        headers_set(&hs->headers_out, "Host", hs->host) < 0) {
        ERROR("Failed to set headers for request");
        return -1;
    }

    // Try to send request
    if (h2_send_request(hs) < 0) {
        ERROR("Failed to send request %s %s", method, uri);
        return -1;
    }

    // Initialize input header list
    header_t header_list_in[HTTP_MAX_HEADER_COUNT];
    headers_init(&hs->headers_in, header_list_in, HTTP_MAX_HEADER_COUNT);

    if (receive_headers(hs) < 0) {
        ERROR("An error ocurred while waiting for server response headers");
        return -1;
    }

    //If it is a GET request, wait for the server response data
    if (strncmp("GET", method, 8) == 0){
      if (receive_server_response_data(hs) < 0) {
          ERROR("An error ocurred while waiting for server response data");
          return -1;
      }
      else if (hs->data_in.size > 0) {
          // Get response data (TODO: should we just copy the pointer?)
          *size = get_data(&hs->data_in, response, *size);
      } else {
        DEBUG("Server response hasn't data");
      }
    }

    int status = atoi(headers_get(&hs->headers_in, ":status"));
    DEBUG("Server replied with status %d", status);

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
        //TODO: should we destroy client socket?
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

int http_head(hstates_t *hs, char *uri, uint8_t *response, size_t *size)
{
    return send_client_request(hs, "HEAD", uri, response, size);
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
