/*
   This API contains the Resource Manager methods to be used by
   HTTP/2
 */

#include <errno.h>
#include <strings.h>
#include <stdio.h>

//#define LOG_LEVEL (LOG_LEVEL_DEBUG)

#include "http.h"
#include "resource_handler.h"
#include "logging.h"

#ifndef MIN
#define MIN(n, m)   (((n) < (m)) ? (n) : (m))
#endif


/*********************************************************
* Private HTTP API methods
*********************************************************/

/**
 * Parse URI into path and query parameters
 *
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
int http_has_method_support(char *method)
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
 * @param    data_in             Buffer with data information
 * @param    data_in_buff_size   Buffer size
 * @param    data_buffer         Buffer for data received
 *
 * @return                       Data lengt
 */
uint32_t get_data(uint8_t *data_in_buff, uint32_t data_in_buff_size, uint8_t *data_buffer, size_t size)
{
    int copysize = MIN(data_in_buff_size, size);

    memcpy(data_buffer, data_in_buff, copysize);
    return copysize;
}

/*
 * Add data to be sent to data lists
 *
 * @param    data_buff           Struct with data information
 * @param    data_buff_size   Buffer size
 * @param    data                Data
 * @param    data_size           Size of data
 *
 * @return   0                   Successfully added data
 * @return   -1                  There was an error in the process
 */
int set_data(uint8_t *data_buff, uint32_t *data_buff_size, uint8_t *data, int data_size)
{
    if (data_size <= 0) {
        ERROR("Data size can't be negative or zero");
        return -1;
    }
    uint32_t min = *data_buff_size;
    if (*data_buff_size > (uint32_t) data_size){
      min = (uint32_t) data_size;
      *data_buff_size = (uint32_t) data_size;
    }
    memcpy(data_buff, data, min);
    return 0;
}

/*
 * Clean data buffer
 *
 * @param    data_buff   Struct with data information
 * @param    data_size   Pointer to buffer size
 *
 * @return   0          Successfully cleaned data buffer
 * @return   -1         There was an error in the process
 */
int clean_data(uint8_t *data_buff, uint32_t *data_size)
{
    memset(data_buff, 0, sizeof(*data_buff));
    *data_size = (uint32_t) 0;
    return 0;
}

/**
 * Send an http error with the given code and message
 */
int error(uint8_t *data_buff, uint32_t *data_size, headers_t *headers_buff, int code, char *msg)
{
    // Set status code
    char strCode[4];

    snprintf(strCode, 4, "%d", code);

    headers_clean(headers_buff);
    headers_set(headers_buff, ":status", strCode);

    // Set error message
    if (msg != NULL) {
        set_data(data_buff, data_size, (uint8_t *)msg, strlen(msg));
    }

    DEBUG("Error with status code %d", code);
    return 0;
}


/************************************
* Server methods
************************************/

/**
 * Perform request for the given method and uri
 */
int do_request(uint8_t *data_buff, uint32_t *data_size, headers_t *headers_buff, char *method, char *uri)
{
    // parse URI removing query parameters
    char path[HTTP_MAX_PATH_SIZE];

    parse_uri(uri, path, NULL);

    // find callback for resource
    http_resource_handler_t handle_uri;
    if ((handle_uri = resource_handler_get(method, path)) == NULL) {
        return error(data_buff, data_size, headers_buff, 404, "Not Found");
    }

    // Clear data buffer in order to prepare response
    clean_data(data_buff, data_size);

    uint8_t response[HTTP_MAX_RESPONSE_SIZE];
    int len;
    if ((len = handle_uri(method, uri, response, HTTP_MAX_RESPONSE_SIZE)) < 0) {
        // if the handler returns
        return error(data_buff, data_size, headers_buff, 500, "Server Error");
    }
    // If it is GET method Prepare response for callback
    else if ((len > 0) && (strncmp("GET", method, 8) == 0)) {
        set_data(data_buff, data_size, response, len);
    }

    // Clean header list
    headers_clean(headers_buff);

    // Set default headers
    headers_set(headers_buff, ":status", "200");

    return 0;
}


int http_server_response(uint8_t *data_buff, uint32_t *data_size, headers_t *headers_buff)
{
    // Get the method, path and scheme from headers
    char *method = headers_get(headers_buff, ":method");
    char *path = headers_get(headers_buff, ":path");

    DEBUG("Received %s request", method);
    if (!http_has_method_support(method)) {
        error(data_buff, data_size, headers_buff, 501, "Not Implemented");
        return 0;
    }

    // TODO: read data (if POST)

    // Process the http request
    return do_request(data_buff, data_size, headers_buff, method, path);
}


/************************************
* Client methods
************************************/


int send_client_request(headers_t *headers_buff, char *method, char *uri, char *host)
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

    return 0;
}


int process_server_response(uint8_t *data_buff, uint32_t data_buff_size, headers_t *headers_buff, char *method, uint8_t *response, size_t *size)
{
    //If it is a GET request, wait for the server response data
    if (strncmp("GET", method, 8) == 0) {
        if (!data_buff_size) {
            DEBUG("Server response hasn't data");
        }
        else if (data_buff_size > 0) {
            // Get response data (TODO: should we just copy the pointer?)
            *size = get_data(data_buff, data_buff_size, response, *size);
        }
    }

    int status = atoi(headers_get(headers_buff, ":status"));
    DEBUG("Server replied with status %d", status);

    return status;
}


int res_manager_get(headers_t *headers_buff, char *uri, uint8_t *response, size_t *size)
{
    (void)response;
    (void)size;
    return send_client_request(headers_buff, "GET", uri, "DEFINIR HOST");
}


int res_manager_head(headers_t *headers_buff, char *uri, uint8_t *response, size_t *size)
{
    (void)response;
    (void)size;
    return send_client_request(headers_buff, "HEAD", uri, "DEFINIR HOST");
}
