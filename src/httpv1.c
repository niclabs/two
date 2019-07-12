#include <stdlib.h>
#include <stdint.h>

#include "http2.h"
#include "sock.h"

typedef enum {
    HTTP_METHOD_GET,
} http_method_t;

typedef struct {
    char name[32];
    char value[128];
} http_header_t;

typedef struct { 
    http_header_t headers[32];
    uint8_t size;
} http_headers_t;

typedef struct {
    http_method_t method;
    char * query_string;
    char * content_type;
    char * content_length;
    http_headers_t * headers;
} http_request_t;

typedef struct {
    int status;
    http_headers_t * headers;
    char * content_type;
    uint8_t * data;
} http_response_t;

typedef int (* http_resource_cb_t) (http_request_t * request, http_response_t * response);

int http_recv_headers() {
    return -1;
}

int http_send_response(http_headers_t * headers, uint8_t * data, int len) {
    return -1;
}

int http_error(uint8_t code, char * msg) {
    return -1;
}

http_resource_cb_t * http_get_resource(char * res) {
    return NULL;
}

int http_set_resource(http_method_t method, char * res, http_resource_cb_t * callback) {
    return -1;
}

int http_server_start(uint16_t port) {
    // initialize server socket
    while (1) {
        // wait for new client
       
        // while http2 connection open 
            // perform http2 initialization for client socket
            // (call h2_server_init_connection)

            // read_headers
            // if method not supported 
            // respond with error 501
            // continue

            // if resource not available
            // respond with error 404
            // continue
            
            // prepare response (pass headers to callback)
            // send response 
        // endwhile
    }
    // close socket
    return 0;
}
