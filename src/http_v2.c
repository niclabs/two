/*
   This API contains the HTTP methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)


#include "http_v2.h"
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
    if ((method == NULL) ||
        ((strncmp("GET", method, 8) != 0) &&
         (strncmp("HEAD", method, 8) != 0))) {
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
 * @param    data_buf   Struct with data information
 * @param    data       Data
 * @param    data_size  Size of data
 *
 * @return   0          Successfully added data
 * @return   -1         There was an error in the process
 */
int set_data(data_t *data_buf, uint8_t *data, int data_size)
{
    if (HTTP_MAX_DATA_SIZE == data_buf->size) {
        ERROR("Data buffer full");
        return -1;
    }
    if (data_size <= 0 || data_size > 128) {
        ERROR("Data too large for buffer size");
        return -1;
    }
    data_buf->size = data_size;
    memcpy(data_buf->buf, data, data_size);
    return 0;
}

/*
 * Clean data buffer
 *
 * @param    data_buf   Struct with data information
 *
 * @return   0          Successfully cleaned data buffer
 * @return   -1         There was an error in the process
 */
int clean_data(data_t *data_buf)
{
    data_buf->size = 0;
    memset(data_buf->buf, 0, sizeof(*data_buf->buf));
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
 * Send an http error with the given code and message
 */
int error(data_t *data_buff, headers_t *headers_buff, int code, char *msg)
{
    // Set status code
    char strCode[4];

    snprintf(strCode, 4, "%d", code);

    headers_clean(headers_buff);
    headers_set(headers_buff, ":status", strCode);

    // Set error message
    if (msg != NULL) {
        clean_data(data_buff);
        set_data(data_buff, (uint8_t *)msg, strlen(msg));
    }

    DEBUG("Error with status code %d", code);
    return 0;
}


/******************************************************************************
 Methods that should be deleted from this layer
*****************************************************************************/



/**
 * Get a resource handler for the given path
 */
http_resource_handler_t get_resource_handler(char *method, char *path)
{
    /*http_resource_t res;

    for (int i = 0; i < hs->resource_list_size; i++) {
        res = hs->resource_list[i];
        if (strncmp(res.path, path, HTTP_MAX_PATH_SIZE) == 0 && strcmp(res.method, method) == 0) {
            return res.handler;
        }
    }*/
    (void) method;
    (void) path;
    return NULL;
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



/****************************************************************************/



/************************************
* Server methods
************************************/

/**
 * Perform request for the given method and uri
 */
int do_request(data_t *data_buff, headers_t *headers_buff, char *method, char *uri)
{
    // parse URI removing query parameters
    char path[HTTP_MAX_PATH_SIZE];

    parse_uri(uri, path, NULL);

    // find callback for resource
    http_resource_handler_t handle_uri;
    if ((handle_uri = get_resource_handler(method, path)) == NULL) {
        return error(data_buff, headers_buff, 404, "Not Found");
    }

    // TODO: response pointer should be pointer to hs->data_out
    uint8_t response[HTTP_MAX_RESPONSE_SIZE];
    int len;
    if ((len = handle_uri(method, uri, response, HTTP_MAX_RESPONSE_SIZE)) < 0) {
        // if the handler returns
        return error(data_buff, headers_buff, 500, "Server Error");
    }
    // If it is GET method Prepare response for callback
    else if ((len > 0) && (strncmp("GET", method, 8) == 0)) {
        clean_data(data_buff);
        set_data(data_buff, response, len);
    }

    // Clean header list
    headers_clean(headers_buff);

    // Set default headers
    headers_set(headers_buff, ":status", "200");

    return 0;
}


int http_server_response(data_t *data_buff, headers_t *headers_buff)
{
    // Get the method, path and scheme from headers
    char *method = headers_get(headers_buff, ":method");
    char *path = headers_get(headers_buff, ":path");

    DEBUG("Received %s request", method);
    if (!has_method_support(method)) {
        error(data_buff, headers_buff, 501, "Not Implemented");
        return 0;
    }

    // TODO: read data (if POST)

    // Process the http request
    do_request(data_buff, headers_buff, method, path);

    return 0;
}


/************************************
* Client methods
************************************/


int send_client_request(headers_t *headers_buff, char *method, char *uri, char *host, uint8_t *response, size_t *size)
{
    // Clean output header list
    headers_clean(headers_buff);

    if (headers_set(headers_buff, ":method", method) < 0 ||
        headers_set(headers_buff, ":scheme", "http") < 0 ||
        headers_set(headers_buff, ":path", uri) < 0 ||
        headers_set(headers_buff, ":host", host) < 0) {
        DEBUG("Failed to set headers for request");
        return -1;
    }

    // Try to send request
    //TODO: this should return to http2 layer
    if (h2_send_request(hs) < 0) {
        DEBUG("Failed to send request %s %s", method, uri);
        return -1;
    }


    // Initialize input header list
    header_t header_list_in[HTTP_MAX_HEADER_COUNT];
    headers_init(&hs->headers_in, header_list_in, HTTP_MAX_HEADER_COUNT);

    /* this is work of http2 layer now
    hs->end_message = 0;
    if (receive_headers(hs) < 0) {
        ERROR("An error ocurred while waiting for server response headers");
        return -1;
    }

    if (hs->connection_state == 0) {
        http_client_disconnect(hs);
        return 0;
    }
    */

    //If it is a GET request, wait for the server response data
    if (strncmp("GET", method, 8) == 0) {
        if (receive_server_response_data(hs) < 0) {
            ERROR("An error ocurred while waiting for server response data");
            return -1;
        }
        else if (hs->data_in.size > 0) {
            // Get response data (TODO: should we just copy the pointer?)
            *size = get_data(&hs->data_in, response, *size);

            if (h2_notify_free_data_buffer(hs, *size) < 0) {
                DEBUG("Error updating window after copying data");
                return -1;
            }
        }
        else {
            DEBUG("Server response hasn't data");
        }
    }

    int status = atoi(headers_get(&hs->headers_in, ":status"));
    DEBUG("Server replied with status %d", status);

    return status;
}


int http_get(hstates_t *hs, char *uri, uint8_t *response, size_t *size)
{
    return send_client_request(hs, "GET", uri, response, size);
}


int http_head(hstates_t *hs, char *uri, uint8_t *response, size_t *size)
{
    return send_client_request(hs, "HEAD", uri, response, size);
}
