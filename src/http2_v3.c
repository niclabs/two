#include <string.h>
#include <assert.h>

#include "frames_v3.h"
#include "http2_v3.h"
#include "list_macros.h"
#include "utils.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"

#define GOAWAY_ERROR(error)                     \
    ({                                          \
        char *errstr;                           \
        switch (error) {                        \
            case HTTP2_NO_ERROR:                \
                errstr = "NO_ERROR";            \
                break;                          \
            case HTTP2_PROTOCOL_ERROR:          \
                errstr = "PROTOCOL_ERROR";      \
                break;                          \
            case HTTP2_INTERNAL_ERROR:          \
                errstr = "INTERNAL_ERROR";      \
                break;                          \
            case HTTP2_FLOW_CONTROL_ERROR:      \
                errstr = "FLOW_CONTROL_ERROR";  \
                break;                          \
            case HTTP2_SETTINGS_TIMEOUT:        \
                errstr = "SETTINGS_TIMEOUT";    \
                break;                          \
            case HTTP2_STREAM_CLOSED_ERROR:     \
                errstr = "STREAM_CLOSED_ERROR"; \
                break;                          \
            case HTTP2_FRAME_SIZE_ERROR:        \
                errstr = "FRAME_SIZE_ERROR";    \
                break;                          \
            case HTTP2_REFUSED_STREAM:          \
                errstr = "REFUSED_STREAM";      \
                break;                          \
            case HTTP2_CANCEL:                  \
                errstr = "CANCEL";              \
                break;                          \
            case HTTP2_COMPRESSION_ERROR:       \
                errstr = "COMPRESSION_ERROR";   \
                break;                          \
            case HTTP2_CONNECT_ERROR:           \
                errstr = "CONNECT_ERROR";       \
                break;                          \
            case HTTP2_ENHANCE_YOUR_CALM:       \
                errstr = "ENHANCE_YOUR_CALM";   \
                break;                          \
            case HTTP2_INADEQUATE_SECURITY:     \
                errstr = "INADEQUATE_SECURITY"; \
                break;                          \
            case HTTP2_HTTP_1_1_REQUIRED:       \
                errstr = "HTTP_1_1_REQUIRED";   \
                break;                          \
            default:                            \
                errstr = "UNKNOWN";             \
                break;                          \
        }                                       \
        errstr;                                 \
    })


#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_FRAME_HEADER_SIZE (9)

// http2 context state flags
#define HTTP2_FLAGS_NONE                 (0x0)
#define HTTP2_FLAGS_WAITING_SETTINGS_ACK (0x1)
#define HTTP2_FLAGS_WAITING_END_HEADERS  (0x2)
#define HTTP2_FLAGS_WAITING_END_STREAM   (0x4)
#define HTTP2_FLAGS_GOAWAY_RECV          (0x8)
#define HTTP2_FLAGS_GOAWAY_SENT          (0x10)

// settings
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE        (0x1)
#define HTTP2_SETTINGS_ENABLE_PUSH              (0x2)
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS   (0x3)
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE      (0x4)
#define HTTP2_SETTINGS_MAX_FRAME_SIZE           (0x5)
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE     (0x6)

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
    ctx->state = HTTP2_CLOSED;
    ctx->stream.state = HTTP2_STREAM_CLOSED;
    event_read_stop(ctx->socket);
    event_close(ctx->socket, http2_on_client_close);
}

int http2_close_gracefully(http2_context_t *ctx)
{
    if (ctx->state == HTTP2_CLOSED || ctx->state == HTTP2_CLOSING) {
        return -1;
    }

    // update state
    ctx->state = HTTP2_CLOSING;
    ctx->flags &= HTTP2_FLAGS_GOAWAY_SENT;
    event_read(ctx->socket, closing);

    // send go away with HTTP2_NO_ERROR
    DEBUG("-> GOAWAY (last_stream_id: %u, error_code: %s)", ctx->last_opened_stream_id, GOAWAY_ERROR(HTTP2_NO_ERROR));
    send_goaway_frame(ctx->socket, HTTP2_NO_ERROR, ctx->last_opened_stream_id, close_on_write_error);

    // TODO: set a timer for goaway

    return 0;
}

void http2_error(http2_context_t *ctx, http2_error_t error)
{
    // update state
    ctx->state = HTTP2_CLOSING;
    ctx->flags &= HTTP2_FLAGS_GOAWAY_SENT;
    event_read(ctx->socket, closing);

    // send goaway with error
    DEBUG("-> GOAWAY (last_stream_id: %u, error_code: %s)", ctx->last_opened_stream_id, GOAWAY_ERROR(error));
    send_goaway_frame(ctx->socket, error, ctx->last_opened_stream_id, close_on_write_error);

    // TODO: set a timer for goaway
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

// update remote settings from settings payload
int update_settings(http2_context_t *ctx, uint8_t *data, int length)
{
    int total = 0;

    for (int i = 0; i < length; i += 6) {
        // read settings id
        uint16_t id = 0;
        id |= data[i + 0] << 8;
        id |= data[i + 1];

        // read settings value
        uint32_t value = 0;
        value |= data[i + 2] << 24;
        value |= data[i + 3] << 16;
        value |= data[i + 4] << 8;
        value |= data[i + 5];

        // update total
        total += 1;
        switch (id) {
            case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
                DEBUG("     - header_table_size: %u", value);
                ctx->settings.header_table_size = value;
                // TODO: update hpack table
                //
                break;
            case HTTP2_SETTINGS_ENABLE_PUSH:
                DEBUG("     - enable_push: %u", value);
                ctx->settings.enable_push = value;
                if (value != 0 && value != 1) {
                    http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                    return 0;
                }
                break;
            case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                ctx->settings.max_concurrent_streams = value;
                break;
            case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                DEBUG("     - initial_window_size: %u", value);
                if (value > (1U << 31) - 1) {
                    http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
                    return 0;
                }
                ctx->settings.initial_window_size = value;
                break;
            case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                DEBUG("     - max_frame_size: %u", value);
                if (value < (1 << 14) || value > ((1 << 24) - 1)) {
                    http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                    return 0;
                }
                ctx->settings.max_frame_size = value;
                break;
            case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                DEBUG("     - max_header_list_size: %u", value);
                ctx->settings.max_header_list_size = value;
                break;
            default:
                // ignore unknown identifiers
                total -= 1;
                break;
        }
    }
    return total;
}

////////////////////////////////////
// begin frame specific handlers
////////////////////////////////////

// Handle a settings frame reception, check for conformance to the
// protocol specification and update remote endpoint settings
int handle_settings_frame(http2_context_t *ctx, frame_header_v3_t header, uint8_t *payload)
{
    // check stream id
    DEBUG("<- SETTINGS (length: %u, flags: 0x%x, stream_id: %u)", header.length, header.flags, header.stream_id);
    if (header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // if not an ack
    if (header.flags == FRAME_NO_FLAG) {
        // check that frame size is divisible by 6
        if (header.length % 6 != 0) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        // process settings
        if (update_settings(ctx, payload, header.length) > 0) {
            DEBUG("-> SETTINGS (ack)" );
            send_settings_frame(ctx->socket, 1, NULL, close_on_write_error);
        }

    }
    else if (header.flags == FRAME_ACK_FLAG) {
        if (header.length != 0) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        // if we are not waiting for settings ack, ignore
        if (!(ctx->flags & HTTP2_FLAGS_WAITING_SETTINGS_ACK)) {
            return 0;
        }

        // disable flag
        ctx->flags &= ~HTTP2_FLAGS_WAITING_SETTINGS_ACK;

        // TODO: disable settings ack timer
    }
    else {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    return 0;
}

void close_on_goaway_reply_sent(event_sock_t *sock, int status)
{
    (void)status;
    http2_context_t *ctx = (http2_context_t *)sock->data;
    http2_close_immediate(ctx);
}

int handle_goaway_frame(http2_context_t *ctx, frame_header_v3_t header, uint8_t *payload)
{
    // check stream id
    DEBUG("<- GOAWAY (length: %u, flags: 0x%x, stream_id: %u)", header.length, header.flags, header.stream_id);
    if (header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // check frame size
    if (header.length < 8) {
        http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return -1;
    }
    uint32_t last_stream_id = bytes_to_uint32_31(payload);
    uint32_t error = bytes_to_uint32(payload + 4);

    // log goaway frame
    DEBUG("     - last_stream_id: %u", last_stream_id);
    DEBUG("     - error_code: %s", GOAWAY_ERROR(error));
    // if sent goaway, close connection immediately
    if (ctx->flags & HTTP2_FLAGS_GOAWAY_SENT) {
        http2_close_immediate(ctx);
        return 0;
    }

    // TODO: if receive goaway, move state to closing and queue goaway
    if (ctx->flags & HTTP2_FLAGS_GOAWAY_RECV) {
        // do nothing here
        return 0;
    }
    else {
        ctx->flags |= HTTP2_FLAGS_GOAWAY_RECV;
        if (ctx->stream.id > last_stream_id) {
            // close current stream
            ctx->stream.state = HTTP2_STREAM_CLOSED;
        }
        // update connection state
        ctx->state = HTTP2_CLOSING;
        event_read(ctx->socket, closing);

        // send goaway and and close connection
        DEBUG("-> GOAWAY (last_stream_id: %u, error_code: %s)", ctx->last_opened_stream_id, GOAWAY_ERROR(HTTP2_NO_ERROR));
        send_goaway_frame(ctx->socket, HTTP2_NO_ERROR, ctx->last_opened_stream_id, close_on_goaway_reply_sent);
    }

    return 0;
}

int handle_ping_frame(http2_context_t *ctx, frame_header_v3_t header, uint8_t *payload)
{
    DEBUG("<- PING (length: %u, flags: 0x%x, stream_id: %u)", header.length, header.flags, header.stream_id);
    // check stream id
    if (header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // check frame size
    if (header.length != 8) {
        http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return -1;
    }

    if (header.flags & FRAME_ACK_FLAG) {
        // ignore ping with ack (we never send pings)
        return 0;
    }

    // send ack with same payload
    DEBUG("-> PING (ack)");
    send_ping_frame(ctx->socket, payload, 1, close_on_write_error);

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
        DEBUG("<- INVALID HTTP2_PREFACE");
        http2_close_immediate(ctx);
        return 24;
    }

    DEBUG("<- HTTP2_PREFACE");

    // update connection state
    ctx->state = HTTP2_WAITING_SETTINGS;

    // prepare settings
    uint32_t settings [] = { HTTP2_HEADER_TABLE_SIZE, HTTP2_ENABLE_PUSH, \
                             HTTP2_MAX_CONCURRENT_STREAMS, HTTP2_INITIAL_WINDOW_SIZE, \
                             HTTP2_MAX_FRAME_SIZE, HTTP2_MAX_HEADER_LIST_SIZE };

    // call send_setting_frame
    DEBUG("-> SETTINGS (length: 36)");
    DEBUG("     - header_table_size: %u", HTTP2_HEADER_TABLE_SIZE);
    DEBUG("     - enable_push: %u", HTTP2_ENABLE_PUSH);
    DEBUG("     - max_concurrent_streams: %u", HTTP2_MAX_CONCURRENT_STREAMS);
    DEBUG("     - initial_window_size: %u", HTTP2_INITIAL_WINDOW_SIZE);
    DEBUG("     - max_frame_size: %u", HTTP2_MAX_FRAME_SIZE);
    DEBUG("     - max_header_list_size: %u", HTTP2_MAX_HEADER_LIST_SIZE);
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
    if (size < HTTP2_FRAME_HEADER_SIZE) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // check the type
    int frame_size = HTTP2_FRAME_HEADER_SIZE + frame_header.length;
    if (frame_header.type != FRAME_SETTINGS_TYPE || frame_header.flags != FRAME_NO_FLAG) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return frame_size; // discard all input after an error
    }

    // do nothing if the frame has not been fully received
    if (size < frame_size) {
        return 0;
    }

    if (handle_settings_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE) < 0) {
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
    if (size < HTTP2_FRAME_HEADER_SIZE) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // do nothing if the frame has not been fully received
    int frame_size = HTTP2_FRAME_HEADER_SIZE + frame_header.length;
    if (size < frame_size) {
        return 0;
    }

    switch (frame_header.type) {
        case FRAME_GOAWAY_TYPE:
            handle_goaway_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        case FRAME_SETTINGS_TYPE:
            handle_settings_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        case FRAME_PING_TYPE:
            handle_ping_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        default:
            DEBUG("-> UNSUPPORTED FRAME TYPE: %d", frame_header.type);
            http2_error(ctx, HTTP2_INTERNAL_ERROR);
            break;
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
    if (size < HTTP2_FRAME_HEADER_SIZE) {
        return 0;
    }

    // read frame
    frame_header_v3_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // do nothing if the frame has not been fully received
    int frame_size = HTTP2_FRAME_HEADER_SIZE + frame_header.length;
    if (size < frame_size) {
        return 0;
    }

    switch (frame_header.type) {
        case FRAME_GOAWAY_TYPE:
            handle_goaway_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        case FRAME_SETTINGS_TYPE:
            handle_settings_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        case FRAME_PING_TYPE:
            handle_ping_frame(ctx, frame_header, buf + HTTP2_FRAME_HEADER_SIZE);
            break;
        default:
            DEBUG("-> UNSUPPORTED FRAME TYPE: %d", frame_header.type);
            http2_error(ctx, HTTP2_INTERNAL_ERROR);
            break;
    }

    return frame_size;
}
