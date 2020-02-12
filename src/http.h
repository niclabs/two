#ifndef HTTP_H
#define HTTP_H

typedef struct http_header
{
    char *name;
    char *value;
} http_header_t;

typedef struct http_request
{
    // request method
    char *method;

    // request path
    char *path;

    // length of http_header list
    unsigned int headers_length;

    // http_header_t list
    http_header_t *headers;
} http_request_t;

typedef struct http_response
{
    // HTTP status code
    int status;

    // value of the Content-Type header
    char *content_type;

    // Length of HTTP response in bytes
    int content_length;

    // response body, it must be allocated
    // by the caller
    char *content;
} http_response_t;

/***********************************************
 * Server API methods
 ***********************************************/

/**
 * Generate a response from server to the given request
 *
 * @param req http request data
 * @param res already allocated http response. The response body will be a
 * buffer with maxlen size
 * @param maxlen maximum size of the http payload
 * @return 0 if ok -1 if an error ocurred
 */
void http_handle_request(http_request_t *req, http_response_t *res,
                         unsigned int maxlen);

/**
 * Utility function to check for method support
 *
 * @returns 1 if the method is supported by the implementation, 0 if not
 */
int http_has_method_support(char *method);

/**
 * Generate an HTTP error response from the server
 *
 * @param res already allocated http response. The response body must be a
 * buffer with maxlen size
 * @param code HTTP error code for the response
 * */
void http_error(http_response_t *res, int code);

#endif /* HTTP_H */
