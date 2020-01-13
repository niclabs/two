/*
   This API contains the "dos" methods to be used by
   HTTP/2
 */

#include <assert.h>

#include "two.h"
#include "event.h"
#include "http2.h"
#include "http.h"

#define LOG_LEVEL LOG_LEVEL_INFO
#include "logging.h"

#include "list_macros.h"

#ifndef TWO_MAX_CLIENTS
#define TWO_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)
#endif

// static variables
static event_loop_t loop;
static event_sock_t *server;
static void (*global_close_cb)();

/*********************************************************
* Private HTTP API methods
*********************************************************/

/**
 * Parse URI into path and query parameters
 *
 * TODO: improve according to https://tools.ietf.org/html/rfc3986
 */
void parse_uri(char *uri, char *path, char *query_params)
{
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
 * Send an http error with the given code and message
 */
void http_error(http_response_t *res, int code, char *msg, unsigned int maxlen)
{
    assert(strlen(msg) < maxlen);

    // prepare response data
    res->status = code;
    memcpy(res->content_type, "text/plain", 10);

    // prepare msg body
    memset(res->body, 0, maxlen);
    if (msg != NULL) {
        memcpy(res->body, msg, strlen(msg));
    }
}

void http_handle_request(http_request_t *req, http_response_t *res, unsigned int maxlen)
{
    if (!http_has_method_support(req->method)) {
        INFO("%s %s HTTP/2 - %d", req->method, req->path, 501);
        http_error(res, 501, "Not Implemented", maxlen);
        return;
    }

    // process the request
    char path[32];
    parse_uri(req->path, path, NULL);

    // find callback for resource
    http_resource_handler_t handle_uri;
    if ((handle_uri = resource_handler_get(req->method, path)) == NULL) {
        http_error(res, 404, "Not Found", maxlen);
        INFO("%s %s HTTP/2 - %d", req->method, req->path, 404);
        return;
    }

    int len;
    char response[maxlen];
    if ((len = handle_uri(req->method, path, (uint8_t *)response, maxlen)) < 0) {
        http_error(res, 500, "Server error", maxlen);
        INFO("%s %s HTTP/2 - %d", req->method, req->path, 500);
        return;
    }

    if ((len > 0) && (strncmp("GET", req->method, 8) == 0)) {
        // TODO: add content_type info to resource
        memcpy(res->content_type, "text/plain", 10);
        memcpy(res->body, response, len);
        res->status = 200;
        res->content_length = len;
    }
    INFO("%s %s HTTP/2 - %d", req->method, req->path, res->status);
}


static void on_server_close(event_sock_t *server)
{
    (void)server;
    INFO("HTTP/2 server closed");
    if (global_close_cb != NULL) {
        global_close_cb();
    }
}

static void on_client_close(event_sock_t *client)
{
    INFO("HTTP/2 client (%d) closed.", client->descriptor);
}

static void on_new_connection(event_sock_t *server, int status)
{
    (void)status;
    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        INFO("New HTTP/2 client connected");
        http2_new_client(client);
    }
    else {
        ERROR("The server cannot receive more clients (current maximum is %d). Increase CONFIG_EVENT_MAX_SOCKETS", EVENT_MAX_SOCKETS);
        event_close(client, on_client_close);
    }
}


int two_server_start(unsigned int port)
{
    event_loop_init(&loop);
    server = event_sock_create(&loop);

    int r = event_listen(server, port, on_new_connection);
    if (r < 0) {
        ERROR("Failed to open socket for listening");
        return 1;
    }
    INFO("Starting HTTP/2 server in port %u", port);
    event_loop(&loop);

    return 0;
}

void two_server_stop(void (*close_cb)())
{
    global_close_cb = close_cb;
    event_close(server, on_server_close);
}

int two_register_resource(char *method, char *path, http_resource_handler_t handler)
{
    return resource_handler_set(method, path, handler);
}
