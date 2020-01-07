#include <string.h>
#include <assert.h>

#include "frames_v3.h"
#include "http2_v3.h"
#include "list_macros.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"


#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_HEADER_SIZE

// http2 context state flags
#define HTTP2_FLAGS_NONE                 (0x0)
#define HTTP2_FLAGS_WAITING_SETTINGS_ACK (0x1)
#define HTTP2_FLAGS_WAITING_END_HEADERS  (0x2)
#define HTTP2_FLAGS_WAITING_END_STREAM   (0x4)

// settings
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE (0x1)
#define HTTP2_SETTINGS_ENABLE_PUSH (0x2)
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS (0x3)
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE (0x4)
#define HTTP2_SETTINGS_MAX_FRAME_SIZE (0x5)
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE (0x6)

#define HTTP2_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)

// static variables for reserved http2 client memory
// and free and connected clients lists
static http2_context_t http2_clients[HTTP2_MAX_CLIENTS];
static http2_context_t *connected_clients;
static http2_context_t *unused_clients;

// default settings from the protocol specification
http2_settings_t default_settings = {
    .header_table_size = 4096,
    .enable_push = 1,
    .max_concurrent_streams = 1,
    .initial_window_size = 65535,
    .max_frame_size = 16384,
    .max_header_list_size = 0xFFFFFFFF
};

// internal definition of a settings frame
// it is useful for easing the reading of settings
// fields from a data array
typedef struct {
    uint8_t header[9];
    struct http2_settings_field {
        uint16_t identifier;
        uint32_t value;
    } __attribute__((packed)) fields[];
} __attribute__((packed)) http2_settings_frame_t;

// used as generic callback for event_write methods. It checks
// the status of the socket and closes the connection if an
// error ocurred
void close_on_write_error(event_sock_t *sock, int status);

// get the first client in the unused_clients list
http2_context_t *find_unused_client();

// return client to unused list
void free_client(http2_context_t *ctx);

// state methods, are called by event read while in a specific
// connection state
int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);
int ready(event_sock_t *client, int size, uint8_t *buf);
int closing(event_sock_t *client, int size, uint8_t *buf);


http2_context_t *http2_new_client(event_sock_t *client)
{
    assert(client != NULL);

    // initialize memory first
    static int inited = 0;
    if (!inited) {
        // Initialize client memory
        memset(http2_clients, 0, HTTP2_MAX_CLIENTS * sizeof(http2_context_t));
        for (int i = 0; i < HTTP2_MAX_CLIENTS - 1; i++) {
            http2_clients[i].next = &http2_clients[i + 1];
        }
        connected_clients = NULL;
        unused_clients = &http2_clients[0];
        inited = 1;
    }

    http2_context_t *ctx = find_unused_client();
    assert(ctx != NULL);

    client->data = ctx;
    ctx->socket = client;
    ctx->settings = default_settings;
    ctx->stream.id = 2; // server side created streams are even
    ctx->stream.state = HTTP2_STREAM_IDLE;
    ctx->state = HTTP2_WAITING_PREFACE;
    ctx->flags = HTTP2_FLAGS_NONE;
    ctx->last_opened_stream_id = 0;

    // TODO: set connection timeout (?)
    event_read_start(client, ctx->read_buf, HTTP2_SOCK_BUF_SIZE, waiting_for_preface);

    return ctx;
}

void http2_on_client_close(event_sock_t *sock)
{
    DEBUG("http/2 client closed");
    http2_context_t *ctx = (http2_context_t *)sock->data;
    free_client(ctx);
}

void http2_close_immediate(http2_context_t *ctx)
{
    assert(ctx->socket != NULL);

    // stop receiving data and close connection
    event_read_stop(ctx->socket);
    event_close(ctx->socket, http2_on_client_close);
}

int http2_close_gracefully(http2_context_t *ctx)
{
    if (ctx->state == HTTP2_CLOSED || ctx->state == HTTP2_CLOSING) {
        return -1;
    }

    // TODO: we should probably do something we already received a goaway frame

    // update state
    ctx->state = HTTP2_CLOSING;
    event_read(ctx->socket, closing);

    // send go away with HTTP2_NO_ERROR
    send_goaway_frame(ctx->socket, HTTP2_NO_ERROR, ctx->last_opened_stream_id, close_on_write_error);

    return 0;
}

void http2_error(http2_context_t *ctx, http2_error_t error)
{
    // log the error
    // if logging is disabled, the switch should be 
    // optimized away by the compiler
    switch(error) {
        case HTTP2_NO_ERROR:
            ERROR("-> GOAWAY: NO_ERROR");
            break;
        case HTTP2_PROTOCOL_ERROR:
            ERROR("-> GOAWAY: PROTOCOL_ERROR");
            break;
        case HTTP2_INTERNAL_ERROR:
            ERROR("-> GOAWAY: INTERNAL_ERROR");
            break;
        case HTTP2_FLOW_CONTROL_ERROR:
            ERROR("-> GOAWAY: FLOW_CONTROL_ERROR");
            break;
        case HTTP2_SETTINGS_TIMEOUT:
            ERROR("-> GOAWAY: SETTINGS_TIMEOUT");
            break;
        case HTTP2_STREAM_CLOSED_ERROR:
            ERROR("-> GOAWAY: STREAM_CLOSED");
            break;
        case HTTP2_FRAME_SIZE_ERROR:
            ERROR("-> GOAWAY: FRAME_SIZE_ERROR");
            break;
        case HTTP2_REFUSED_STREAM:
            ERROR("-> GOAWAY: REFUSED_STREAM");
            break;
        case HTTP2_CANCEL:
            ERROR("-> GOAWAY: CANCEL");
            break;
        case HTTP2_COMPRESSION_ERROR:
            ERROR("-> GOAWAY: COMPRESSION_ERROR");
            break;
        case HTTP2_CONNECT_ERROR:
            ERROR("-> GOAWAY: CONNECT_ERROR");
            break;
        case HTTP2_ENHANCE_YOUR_CALM:
            ERROR("-> GOAWAY: ENHANCE_YOUR_CALM");
            break;
        case HTTP2_INADEQUATE_SECURITY:
            ERROR("-> GOAWAY: INADEQUATE_SECURITY");
            break;
        case HTTP2_HTTP_1_1_REQUIRED:
            ERROR("-> GOAWAY: HTTP_1_1_REQUIRED");
            break;
    }
    
    // update state
    ctx->state = HTTP2_CLOSING;
    event_read(ctx->socket, closing);

    // send goaway with error
    send_goaway_frame(ctx->socket, error, ctx->last_opened_stream_id, close_on_write_error);
}

void close_on_write_error(event_sock_t *sock, int status)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (status < 0) {
        http2_close_immediate(ctx);
    }
}

void on_settings_sent(event_sock_t *sock, int status)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (status < 0) {
        http2_close_immediate(ctx);
        return;
    }
    // set waiting for ack flag to true
    ctx->flags |= HTTP2_FLAGS_WAITING_SETTINGS_ACK;

    // TODO: set ack timer
}

// find a free client in the
http2_context_t *find_unused_client()
{
    http2_context_t *ctx = LIST_POP(unused_clients);

    if (ctx != NULL) {
        memset(ctx, 0, sizeof(http2_context_t));
        LIST_PUSH(ctx, connected_clients);
    }

    return ctx;
}

void free_client(http2_context_t *ctx)
{
    if (LIST_DELETE(ctx, connected_clients) != NULL) {
        LIST_PUSH(ctx, unused_clients);
    }
}

// update remote settings from frame data
void update_settings(http2_context_t *ctx, uint8_t *data, unsigned int size)
{
    // cast the buffer as a settings frame
    http2_settings_frame_t *frame = (http2_settings_frame_t *)data;

    int len = (size - 9) / sizeof(struct http2_settings_field);

    DEBUG("<- SETTINGS");
    for (int i = 0; i < len; i++) {
        uint16_t id = frame->fields[i].identifier;
        uint32_t value = frame->fields[i].value;
        switch (id) {
            case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
                DEBUG("     header_table_size: %d", value);
                ctx->settings.header_table_size = value;
                // TODO: update hpack table
                break;
            case HTTP2_SETTINGS_ENABLE_PUSH:
                DEBUG("     enable_push: %d", value);
                ctx->settings.enable_push = value;
                break;
            case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                ctx->settings.max_concurrent_streams = value;
                break;
            case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                DEBUG("     initial_window_size: %d", value);
                if (value > (1U << 31) - 1) {
                    http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
                    return;
                }
                ctx->settings.initial_window_size = value;
                break;
            case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                DEBUG("     max_frame_size: %d", value);
                if (value < (1 << 14) || value > ((1 << 24) - 1)) {
                    http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                    return;
                }
                ctx->settings.max_frame_size = value;
                break;
            case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                DEBUG("     max_header_list_size: %d", value);
                ctx->settings.max_header_list_size = value;
                break;
            default:
                break;
        }
    }

}

////////////////////////////////////
// begin frame specific handlers
////////////////////////////////////

// Handle a settings frame reception, check for conformance to the
// protocol specification and update remote endpoint settings
// data and size must respectively contain the full frame
// data buffer and the exact frame size (i.e. 9 + header_length)
int handle_settings_frame(http2_context_t *ctx, frame_header_v3_t frame_header, uint8_t *data, unsigned int size)
{
    // check stream id
    if (frame_header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // if not an ack
    if (frame_header.flags == FRAME_NO_FLAG) {
        // check that frame size is divisible by 6
        if (frame_header.length % 6 != 0) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        update_settings(ctx, data, size);

        DEBUG("-> SETTINGS: ACK");
        send_settings_frame(ctx->socket, 1, NULL, close_on_write_error);
    }
    else if (frame_header.flags == FRAME_ACK_FLAG) {
        if (frame_header.length != 0) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        // if we are not waiting for settings ack
        // send a protocol error
        if (!(ctx->flags & HTTP2_FLAGS_WAITING_SETTINGS_ACK)) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        DEBUG("<- SETTINGS: ACK");
        ctx->flags &= ~HTTP2_FLAGS_WAITING_SETTINGS_ACK;

        // TODO: disable settings ack timer
    }
    else {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    return 0;
}

int handle_goaway_frame(http2_context_t *ctx, frame_header_v3_t frame_header, uint8_t *data, unsigned int size)
{
    // check stream id
    if (frame_header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // check frame size
    if (frame_header.length < 8) {
        http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return -1;
    }

    return 0;
}

////////////////////////////////////
// begin server state methods
////////////////////////////////////


// Handle read operations while waiting for a preface
// if anything else than a preface is received in this state
// a PROTOCOL_ERROR is sent
int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf)
{
    assert(client->data != NULL);

    http2_context_t *ctx = (http2_context_t *)client->data;
    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }

    // Wait until preface is received
    if (size < 24) {
        return 0;
    }

    if (strncmp((char *)buf, HTTP2_PREFACE, 24) != 0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return 24;
    }

    DEBUG("<- HTTP/2 PREFACE");

    // update connection state
    ctx->state = HTTP2_WAITING_SETTINGS;

    // prepare settings
    uint32_t settings [] = { HTTP2_HEADER_TABLE_SIZE, HTTP2_ENABLE_PUSH, \
                             HTTP2_MAX_CONCURRENT_STREAMS, HTTP2_INITIAL_WINDOW_SIZE, \
                             HTTP2_MAX_FRAME_SIZE, HTTP2_MAX_HEADER_LIST_SIZE };

    // call send_setting_frame
    DEBUG("-> SETTINGS");
    DEBUG("     header_table_size: %d", HTTP2_HEADER_TABLE_SIZE);
    DEBUG("     enable_push: %d", HTTP2_ENABLE_PUSH);
    DEBUG("     max_concurrent_streams: %d", HTTP2_MAX_CONCURRENT_STREAMS);
    DEBUG("     initial_window_size: %d", HTTP2_INITIAL_WINDOW_SIZE);
    DEBUG("     max_frame_size: %d", HTTP2_MAX_FRAME_SIZE);
    DEBUG("     max_header_list_size: %d", HTTP2_MAX_HEADER_LIST_SIZE);
    send_settings_frame(client, 0, settings, on_settings_sent);

    // go to next state
    event_read(client, waiting_for_settings);

    return 24;
}

// Handle read operations while waiting for a settings frame
// if anything else than a non-ACK settings frame is received
// a protocol error is returned
int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }

    // Wait until settings header is received
    if (size < 9) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // check the type
    if (frame_header.type != FRAME_SETTINGS_TYPE || frame_header.flags != FRAME_NO_FLAG) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return size; // discard all input after an error
    }

    int frame_size = 9 + frame_header.length;
    // do nothing if the frame has not been fully received
    if (size < frame_size) {
        return 0;
    }

    if (handle_settings_frame(ctx, frame_header, buf, frame_size) < 0) {
        // if an error ocurred, discard the remaining data
        return size;
    }

    // go to next state
    ctx->state = HTTP2_READY;
    event_read(sock, ready);

    return frame_size;
}

// Handle read operations while the server is in a ready state
// i.e. after settings exchange
// most frame operations will occur while in this state
int ready(event_sock_t *sock, int size, uint8_t *buf)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }

    // Wait until frame header is received
    if (size < 9) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // do nothing if the frame has not been fully received
    int frame_size = 9 + frame_header.length;
    if (size < frame_size) {
        return 0;
    }


    int rc = 0;
    switch (frame_header.type) {
        case FRAME_GOAWAY_TYPE:
            // TODO: handle goaway
            break;
        case FRAME_SETTINGS_TYPE:
            // check stream id
            rc = handle_settings_frame(ctx, frame_header, buf, frame_size);
            break;
        default:
            http2_error(ctx, HTTP2_INTERNAL_ERROR);
            return size;
    }

    if (rc < 0) {
        return size;
    }

    return frame_size;
}

// Handle read operations after a GOAWAY frame is sent
// or received. No new streams are able to be created
// at this point, but pending operations will be terminated
int closing(event_sock_t *sock, int size, uint8_t *buf)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }

    // Wait until frame header is received
    if (size < 9) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // do nothing if the frame has not been fully received
    int frame_size = 9 + frame_header.length;
    if (size < frame_size) {
        return 0;
    }


    switch (frame_header.type) {
        case FRAME_GOAWAY_TYPE:
            // TODO: handle goaway
            break;
        default:
            http2_error(ctx, HTTP2_INTERNAL_ERROR);
            return size;
    }

    return frame_size;
}
