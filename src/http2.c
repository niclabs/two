#include "http2.h"

#include <assert.h>
#include <string.h>
#include <strings.h>

#include "frames.h"
#include "http.h"
#include "ll.h"
#include "utils.h"

#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

#define HTTP2_PREFACE           "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_FRAME_HEADER_SIZE (9)

// http2 context state flags
#define HTTP2_FLAGS_NONE                 (0x0)
#define HTTP2_FLAGS_WAITING_SETTINGS_ACK (0x1)
#define HTTP2_FLAGS_WAITING_END_HEADERS  (0x2)
#define HTTP2_FLAGS_WAITING_END_STREAM   (0x4)
#define HTTP2_FLAGS_GOAWAY_RECV          (0x8)
#define HTTP2_FLAGS_GOAWAY_SENT          (0x10)
#define HTTP2_FLAGS_WAITING_TRAILERS     (0x20)

// settings
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE      (0x1)
#define HTTP2_SETTINGS_ENABLE_PUSH            (0x2)
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS (0x3)
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE    (0x4)
#define HTTP2_SETTINGS_MAX_FRAME_SIZE         (0x5)
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE   (0x6)

#ifndef HTTP2_MAX_CLIENTS
#define HTTP2_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)
#endif

#undef MIN
#define MIN(x, y) (x) < (y) ? (x) : (y)

// static variables for reserved http2 client memory
// and free and connected clients lists
LL_STATIC(http2_context_t, clients, HTTP2_MAX_CLIENTS);
static http2_context_t *connected_clients;

// Current client id
static uint8_t client_id;

// default settings from the protocol specification
http2_settings_t default_settings = { .header_table_size      = 4096,
                                      .enable_push            = 1,
                                      .max_concurrent_streams = 1,
                                      .initial_window_size    = 65535,
                                      .max_frame_size         = 16384,
                                      .max_header_list_size   = 0xFFFFFFFF };

// used as generic callback for event_write methods. It checks
// the status of the socket and closes the connection if an
// error ocurred
void close_on_write_error(event_sock_t *sock, int status);
void close_on_goaway_sent(event_sock_t *sock, int status);

// state methods, are called by event read while in a specific
// connection state
int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);
int receiving(event_sock_t *client, int size, uint8_t *buf);

// send as much data as flow control allows from the stream buffer
void http2_continue_send(http2_context_t *ctx, http2_stream_t *stream);
void http2_on_client_close(event_sock_t *sock);

http2_context_t *http2_new_client(event_sock_t *client)
{
    assert(client != NULL);

    // initialize memory first
    static int inited = 0;
    if (!inited) {
        // Initialize client memory
        LL_INIT(clients, HTTP2_MAX_CLIENTS);
        connected_clients = NULL;
        inited            = 1;
    }

    // get first element from the client list into the connected clients list
    http2_context_t *ctx = LL_MOVE(clients, connected_clients);
    if (ctx == NULL) {
        // increase CONFIG_HTTP2_MAX_CLIENTS to avoid this error
        ERROR("Maximum number of clients (%d) reached.", HTTP2_MAX_CLIENTS);
        event_close(client, http2_on_client_close);
        return NULL;
    }
    INFO("http/2 client %d connected", client_id);

    client->data               = ctx;
    ctx->id                    = client_id++;
    ctx->socket                = client;
    ctx->settings              = default_settings;
    ctx->stream.id             = 0;
    ctx->stream.state          = HTTP2_STREAM_IDLE;
    ctx->state                 = HTTP2_WAITING_PREFACE;
    ctx->flags                 = HTTP2_FLAGS_NONE;
    ctx->last_opened_stream_id = 0;

    // this value can only be updated by a WINDOW_UPDATE frame
    ctx->window_size = default_settings.initial_window_size;

    // initialize hpack
    hpack_init(&ctx->hpack_dynamic_table, HTTP2_HEADER_TABLE_SIZE);

    event_read_start(client, ctx->read_buf, HTTP2_SOCK_READ_SIZE,
                     waiting_for_preface);
    event_write_enable(client, ctx->write_buf, HTTP2_SOCK_WRITE_SIZE);

    return ctx;
}

void http2_on_client_close(event_sock_t *sock)
{
    if (sock->data != NULL) {
        http2_context_t *ctx = (http2_context_t *)sock->data;
        INFO("http/2 client %u disconnected", ctx->id);

        // free the client
        LL_DELETE(ctx, connected_clients);
        LL_PUSH(ctx, clients);
    } else {
        INFO("http/2 client disconnected");
    }
}

void http2_close_immediate(http2_context_t *ctx)
{
    assert(ctx->socket != NULL);

    // stop receiving data and close connection
    ctx->state        = HTTP2_CLOSED;
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

    // send go away with HTTP2_NO_ERROR
    INFO("->|%u| GOAWAY (last_stream_id: %u, error_code: 0x%x)", ctx->id,
         (unsigned int)ctx->last_opened_stream_id, HTTP2_NO_ERROR);
    send_goaway_frame(ctx->socket, HTTP2_NO_ERROR, ctx->last_opened_stream_id,
                      close_on_write_error);

    // TODO: set a timer for goaway (?)

    return 0;
}

void http2_error(http2_context_t *ctx, http2_error_t error)
{
    // update state
    ctx->state = HTTP2_CLOSING;
    ctx->flags &= HTTP2_FLAGS_GOAWAY_SENT;

    // stop accepting data
    event_read_stop(ctx->socket);

    // send goaway and close the connection
    INFO("->|%u| GOAWAY (last_stream_id: %u, error_code: 0x%x)", ctx->id,
         (unsigned int)ctx->last_opened_stream_id, error);
    send_goaway_frame(ctx->socket, error, ctx->last_opened_stream_id,
                      close_on_goaway_sent);
}

void http2_stream_error(http2_context_t *ctx, uint32_t stream_id,
                        http2_error_t error)
{
    // Send reset stream and close frame
    INFO("->|%u| RST_STREAM (stream_id: %u, error_code: 0x%x)", ctx->id,
         (unsigned int)stream_id, error);
    send_rst_stream_frame(ctx->socket, error, stream_id, close_on_write_error);
    if (stream_id == ctx->stream.id) {
        ctx->stream.state = HTTP2_STREAM_CLOSED;
    }
}

void close_on_write_error(event_sock_t *sock, int status)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (status < 0) {
        http2_close_immediate(ctx);
    }
}

void close_on_goaway_sent(event_sock_t *sock, int status)
{
    (void)status;
    http2_context_t *ctx = (http2_context_t *)sock->data;
    http2_close_immediate(ctx);
}

int on_settings_timeout(event_sock_t *sock)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    http2_error(ctx, HTTP2_SETTINGS_TIMEOUT);
    return 1;
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

    // set ack timer
    ctx->timer = event_timer(sock, HTTP2_SETTINGS_WAIT, on_settings_timeout);
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
                DEBUG("     - header_table_size: %u", (unsigned int)value);
                ctx->settings.header_table_size = value;

                // TODO: confirm what happens if my header table size is lower
                // than the remote endpoint header table size
                hpack_dynamic_change_max_size(
                  &ctx->hpack_dynamic_table,
                  MIN(HTTP2_HEADER_TABLE_SIZE, value));
                break;
            case HTTP2_SETTINGS_ENABLE_PUSH:
                DEBUG("     - enable_push: %u", (unsigned int)value);
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
                DEBUG("     - initial_window_size: %u", (unsigned int)value);
                if (value > (1U << 31) - 1) {
                    http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
                    return 0;
                }

                // A SETTINGS frame can alter the initial flow-control window
                // size for streams with active flow-control windows. When the
                // value of SETTINGS_INITIAL_WINDOW_SIZE changes, a receiver
                // MUST adjust the size of all stream flow-control windows that
                // it maintains by the difference between the new value and the
                // old value.
                if (ctx->stream.state == HTTP2_STREAM_OPEN ||
                    ctx->stream.state == HTTP2_STREAM_HALF_CLOSED_REMOTE) {
                    int32_t diff = value - ctx->settings.initial_window_size;

                    // check max window size
                    if (ctx->stream.window_size + diff > 0 &&
                        (unsigned)(ctx->stream.window_size + diff) >
                          ((uint32_t)(1 << 31) - 1)) {
                        http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
                        return 0;
                    }
                    ctx->stream.window_size += diff;
                }
                ctx->settings.initial_window_size = value;
                break;
            case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                DEBUG("     - max_frame_size: %u", (unsigned int)value);
                if (value < (1 << 14) || value > ((1 << 24) - 1)) {
                    http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                    return 0;
                }
                ctx->settings.max_frame_size = value;
                break;
            case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                DEBUG("     - max_header_list_size: %u", (unsigned int)value);
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
int handle_settings_frame(http2_context_t *ctx, frame_header_t header,
                          uint8_t *payload)
{
    // check stream id
    INFO("<-|%u| SETTINGS (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);
    if (header.stream_id != 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // if not an ack
    if (!(header.flags & FRAME_FLAGS_ACK)) {
        // check that frame size is divisible by 6
        if (header.length % 6 != 0) {
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return -1;
        }

        // process settings
        if (update_settings(ctx, payload, header.length) > 0) {
            INFO("->|%u| SETTINGS (ack)", ctx->id);
            send_settings_frame(ctx->socket, 1, NULL, close_on_write_error);
        }

    } else {
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

        // disable settings ack timer
        event_timer_stop(ctx->timer);
        ctx->timer = NULL;
    }

    return 0;
}

// Handle a goaway frame reception, check for conformance to the specificataion
// and go to closing state
int handle_goaway_frame(http2_context_t *ctx, frame_header_t header,
                        uint8_t *payload)
{
    // check stream id
    INFO("<-|%u| GOAWAY (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);
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

    // log goaway frame
    DEBUG("     - last_stream_id: %u", (unsigned int)last_stream_id);
    DEBUG("     - error_code: 0x%x",
          (unsigned int)bytes_to_uint32(payload + 4));
    // if sent goaway, close connection immediately
    if (ctx->flags & HTTP2_FLAGS_GOAWAY_SENT) {
        http2_close_immediate(ctx);
        return 0;
    }

    // if goaway received, move state to closing and queue goaway
    if (ctx->flags & HTTP2_FLAGS_GOAWAY_RECV) {
        // do nothing here
        return 0;
    } else {
        ctx->flags |= HTTP2_FLAGS_GOAWAY_RECV;
        if (ctx->stream.id > last_stream_id) {
            // close current stream
            ctx->stream.state = HTTP2_STREAM_CLOSED;
        }
        // update connection state
        ctx->state = HTTP2_CLOSING;
        event_read(ctx->socket, receiving);

        // send goaway and and close connection
        INFO("->|%u| GOAWAY (last_stream_id: %u, error_code: 0x%x)", ctx->id,
             (unsigned int)ctx->last_opened_stream_id, HTTP2_NO_ERROR);
        send_goaway_frame(ctx->socket, HTTP2_NO_ERROR,
                          ctx->last_opened_stream_id, close_on_goaway_sent);
    }

    return 0;
}

// Handle a ping frame reception. Reply with same payload and an ack if the
// frame is well formed
int handle_ping_frame(http2_context_t *ctx, frame_header_t header,
                      uint8_t *payload)
{
    INFO("<-|%u| PING (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);
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

    if (header.flags & FRAME_FLAGS_ACK) {
        // ignore ping with ack (we never send pings)
        return 0;
    }

    // send ack with same payload
    INFO("->|%u| PING (ack)", ctx->id);
    send_ping_frame(ctx->socket, payload, 1, close_on_write_error);

    return 0;
}

////////////////////////////////////////////////////////////////
// Headers and continuation functions
////////////////////////////////////////////////////////////////

int validate_pseudoheaders(header_list_t *headers)
{
    char *tests[3] = { ":method", ":scheme", ":path" };

    for (int i = 0; i < 3; i++) {
        char *val = header_list_get(headers, tests[i]);
        // check non existent header
        if (val == NULL) {
            return -1;
        }

        // check empty header
        if (strlen(val) == 0) {
            return -1;
        }

        // check duplicated header
        if (index(val, ',') != NULL) {
            return -1;
        }
    }

    // check connection-specific Header Fields
    char *connection = header_list_get(headers, "connection");
    if (connection != NULL) {
        DEBUG("connection: %s", connection);
        // An endpoint MUST NOT generate an HTTP/2 message containing
        // connection-specific header fields; any message containing
        // connection-specific header fields MUST be treated as
        // malformed (Section 8.1.2.6).
        return -1;
    }

    char *te = header_list_get(headers, "te");
    if (te != NULL && strcmp(te, "trailers") != 0) {
        // The only exception to this is the TE header field,
        // which MAY be present in an HTTP/2 request; when it is,
        // it MUST NOT contain any value other than "trailers".
        return -1;
    }

    return 0;
}

void on_stream_send_complete(event_sock_t *sock, int status)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (status < 0) {
        http2_close_immediate(ctx);
        return;
    }

    if (ctx->stream.buflen <= 0) {
        // close the stream if we send all available data
        ctx->stream.state = HTTP2_STREAM_CLOSED;
    } else {
        // else send remaining data
        http2_continue_send(ctx, &ctx->stream);
    }
}

void http2_continue_send(http2_context_t *ctx, http2_stream_t *stream)
{
    // send at most window_size
    int window_size = MIN(ctx->window_size, stream->window_size);
    int len         = MIN(stream->buflen, window_size);

    // do nothing if window size is lower than 0
    if (len <= 0) {
        return;
    }

    // limit size to write buffer
    len = MIN(len, HTTP2_SOCK_WRITE_SIZE);

    // send data frame
    INFO("->|%u| DATA (length: %u, flags: 0x%x, stream_id: %u)", ctx->id, len,
         (unsigned int)stream->id, (stream->buflen - len <= 0));
    send_data_frame(ctx->socket, stream->bufptr, len, stream->id,
                    (stream->buflen - len <= 0), on_stream_send_complete);

    // use actual size sent here
    stream->bufptr += len;
    stream->buflen -= len;
    stream->window_size -= len;
    ctx->window_size -= len;
}

int handle_window_update_frame(http2_context_t *ctx, frame_header_t header,
                               uint8_t *payload)
{
    INFO("<-|%u| WINDOW_UPDATE (length: %u, flags: 0x%x, stream_id: %u)",
         ctx->id, header.length, header.flags, header.stream_id);

    // check header length
    if (header.length != 4) {
        http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return -1;
    }

    // check stream id and stream state
    if (header.stream_id > ctx->last_opened_stream_id) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    if (header.stream_id != 0 && header.stream_id == ctx->stream.id &&
        ctx->stream.state == HTTP2_STREAM_CLOSED) {
        http2_error(ctx, HTTP2_STREAM_CLOSED_ERROR);
        return -1;
    }

    uint32_t window_size_increment = bytes_to_uint32_31(payload);
    DEBUG("     - window_size_increment: %u",
          (unsigned int)window_size_increment);
    if (window_size_increment == 0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // update connection window size
    if (header.stream_id == 0) {
        if (ctx->window_size + window_size_increment >
            ((uint32_t)(1 << 31) - 1)) {
            http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
            return -1;
        }
        ctx->window_size += window_size_increment;
    } else { // update stream window size
        if (ctx->stream.window_size + window_size_increment >
            ((uint32_t)(1 << 31) - 1)) {
            http2_stream_error(ctx, ctx->stream.id, HTTP2_FLOW_CONTROL_ERROR);
            return -1;
        }
        ctx->stream.window_size += window_size_increment;

        // end remaining data
        http2_continue_send(ctx, &ctx->stream);
    }

    return 0;
}

int handle_end_stream(http2_context_t *ctx, http2_stream_t *stream)
{
    // decode header block
    header_list_t header_list;

    header_list_reset(&header_list);

    switch (hpack_decode(&ctx->hpack_dynamic_table, stream->buf, stream->buflen,
                         &header_list)) {
        case HPACK_COMPRESSION_ERROR:
            http2_error(ctx, HTTP2_COMPRESSION_ERROR);
            return -1;
        case HPACK_MEMORY_ERROR:
            http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
            return -1;
        case HPACK_INTERNAL_ERROR:
            http2_error(ctx, HTTP2_INTERNAL_ERROR);
            return -1;
        default:
            break;
    }

    if (validate_pseudoheaders(&header_list) < 0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // handle request at end headers
    // data frames are ignored
    // prepare http request
    int headers_length = header_list_count(&header_list);
    http_header_t headers[headers_length];

    http_request_t req = { .method = header_list_get(&header_list, ":method"),
                           .path   = header_list_get(&header_list, ":path"),
                           .headers_length = headers_length,
                           .headers = header_list_all(&header_list, headers) };

    http_response_t res = { .content = (char *)stream->buf };
    http_handle_request(&req, &res, HTTP2_STREAM_BUF_SIZE);

    // prepare HTTP2 headers
    header_list_reset(&header_list);

    // status code
    char strCode[4];
    snprintf(strCode, 4, "%d", res.status);
    header_list_set(&header_list, ":status", strCode);

    // content type and length
    header_list_set(&header_list, "content-type", res.content_type);

    char strLen[10]; // 10 digits to be safe
    snprintf(strLen, 10, "%d", res.content_length);
    header_list_set(&header_list, "content-length", strLen);

    // Response data goes to the stream buffer
    stream->buflen = res.content_length;

    // send headers
    int hlen = 0;
    if ((hlen = send_headers_frame(
           ctx->socket, &header_list, &ctx->hpack_dynamic_table, stream->id,
           stream->buflen == 0, on_stream_send_complete)) < 0) {
        http2_error(ctx, HTTP2_INTERNAL_ERROR);
        return -1;
    }
    INFO("->|%u| HEADERS (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         hlen,
         FRAME_FLAGS_END_HEADERS |
           (stream->buflen > 0 ? 0 : FRAME_FLAGS_END_STREAM),
         (unsigned int)stream->id);

    return 0;
}

int handle_header_block(http2_context_t *ctx, frame_header_t header,
                        uint8_t *data, int size)
{
    // copy header data to stream buffer
    int copylen = MIN(size, HTTP2_STREAM_BUF_SIZE - ctx->stream.buflen);

    // handle buffer full
    if (copylen <= 0) {
        http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
        return -1;
    }

    // copy memory to the stream buffer
    if (!(ctx->flags & HTTP2_FLAGS_WAITING_TRAILERS)) {
        memcpy(ctx->stream.buf + ctx->stream.buflen, data, copylen);
        ctx->stream.buflen += copylen;
    }

    if (header.flags & FRAME_FLAGS_END_STREAM) {
        // set the stream to the correct state
        ctx->flags &= ~HTTP2_FLAGS_WAITING_END_STREAM;
        ctx->stream.state = HTTP2_STREAM_HALF_CLOSED_REMOTE;
    }

    if (header.flags & FRAME_FLAGS_END_HEADERS) {
        // no longer waiting for headers, we need to process request
        ctx->flags &= ~HTTP2_FLAGS_WAITING_END_HEADERS;
    }

    // if we received both end headers and end stream
    // start sending data
    if (!(ctx->flags & HTTP2_FLAGS_WAITING_END_HEADERS) &&
        !(ctx->flags & HTTP2_FLAGS_WAITING_END_STREAM)) {
        return handle_end_stream(ctx, &ctx->stream);
    }
    return 0;
}

int handle_headers_frame(http2_context_t *ctx, frame_header_t header,
                         uint8_t *payload)
{
    // ignore new streams after starting close
    if (ctx->state == HTTP2_CLOSING) {
        INFO("X-|%u| HEADERS (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
             header.length, header.flags, header.stream_id);
        return 0;
    }

    INFO("<-|%u| HEADERS (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);
    // check stream id and stream state
    if (header.stream_id == 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // stream id must be bigger than previous
    // odd numbers must be used for client initiated streams
    if (header.stream_id < ctx->last_opened_stream_id ||
        header.stream_id % 2 == 0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // client is trying to open a new stream when the old
    // one has not been closed yet
    if (header.stream_id > ctx->last_opened_stream_id &&
        ctx->stream.state != HTTP2_STREAM_IDLE) {
        http2_error(ctx, HTTP2_REFUSED_STREAM);
        return -1;
    }

    // new headers frame for a strean in HALF_CLOSED_REMOTE
    // TODO: check stream state and handle trailing headers
    if (header.stream_id == ctx->last_opened_stream_id &&
        (ctx->stream.state == HTTP2_STREAM_HALF_CLOSED_REMOTE ||
         ctx->stream.state == HTTP2_STREAM_CLOSED)) {
        http2_error(ctx, HTTP2_STREAM_CLOSED_ERROR);
        return -1;
    }

    // Header trailers.
    if (header.stream_id == ctx->last_opened_stream_id) {
        if (!(header.flags & FRAME_FLAGS_END_STREAM)) {
            http2_stream_error(ctx, header.stream_id, HTTP2_PROTOCOL_ERROR);
            return -1;
        }

        ctx->flags |= HTTP2_FLAGS_WAITING_TRAILERS;
        ctx->flags |= HTTP2_FLAGS_WAITING_END_HEADERS;
    } else {
        // open a new stream
        ctx->stream.id             = header.stream_id;
        ctx->stream.state          = HTTP2_STREAM_OPEN;
        ctx->stream.window_size    = ctx->settings.initial_window_size;
        ctx->last_opened_stream_id = header.stream_id;

        // reset stream flags
        ctx->flags |= HTTP2_FLAGS_WAITING_END_HEADERS;
        ctx->flags |= HTTP2_FLAGS_WAITING_END_STREAM;

        // initialize stream buffer
        ctx->stream.buflen = 0;
        ctx->stream.bufptr = ctx->stream.buf;
    }

    // calculate header payload size
    int size = header.length;
    if (header.flags & FRAME_FLAGS_PADDED) {
        // Padding that exceeds remaining size for header block
        // must be treated as PROTOCOL_ERROR
        if (*payload >= header.length) {
            http2_error(ctx, HTTP2_PROTOCOL_ERROR);
            return -1;
        }

        // 1 byte (padding from the total size)
        size -= 1;

        // remove the payload size from the total size
        size -= *payload;
        payload++;
    }

    // ignore all priority data
    if (header.flags & FRAME_FLAGS_PRIORITY) {
        size -= 5; // remove the priority size from total size
        payload += 5;
    }

    // update headers from the block size
    return handle_header_block(ctx, header, payload, size);
}

// this checks DATA frames for end_stream flag
// it will ignore all the data
int handle_data_frame(http2_context_t *ctx, frame_header_t header,
                      uint8_t *payload)
{
    INFO("X-|%u| DATA (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);

    // check stream id
    if (header.stream_id == 0x0 ||
        header.stream_id > ctx->last_opened_stream_id) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    if (header.stream_id < ctx->last_opened_stream_id) {
        http2_stream_error(ctx, header.stream_id, HTTP2_STREAM_CLOSED_ERROR);
        return -1;
    }

    if (ctx->stream.state != HTTP2_STREAM_OPEN) {
        http2_stream_error(ctx, header.stream_id, HTTP2_STREAM_CLOSED_ERROR);
        return -1;
    }

    if (header.flags & FRAME_FLAGS_PADDED) {
        // Padding that exceeds remaining size for header block
        // must be treated as PROTOCOL_ERROR
        if (*payload >= header.length) {
            http2_error(ctx, HTTP2_PROTOCOL_ERROR);
            return -1;
        }
    }

    if (header.flags & FRAME_FLAGS_END_STREAM) {
        // set the stream to the correct state
        ctx->flags &= ~HTTP2_FLAGS_WAITING_END_STREAM;
        ctx->stream.state = HTTP2_STREAM_HALF_CLOSED_REMOTE;
    }

    // if we received both end headers and end stream
    // start sending data
    if (!(ctx->flags & HTTP2_FLAGS_WAITING_END_HEADERS) &&
        !(ctx->flags & HTTP2_FLAGS_WAITING_END_STREAM)) {
        handle_end_stream(ctx, &ctx->stream);
    }

    return 0;
}
int handle_continuation_frame(http2_context_t *ctx, frame_header_t header,
                              uint8_t *payload)
{
    INFO("<-|%u| CONTINUATION (length: %u, flags: 0x%x, stream_id: %u)",
         ctx->id, header.length, header.flags, header.stream_id);

    // check stream id and stream state
    if (header.stream_id == 0x0 ||
        (ctx->stream.state != HTTP2_STREAM_OPEN &&
         ctx->stream.state != HTTP2_STREAM_HALF_CLOSED_REMOTE) ||
        !(ctx->flags & HTTP2_FLAGS_WAITING_END_HEADERS)) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // update headers from the block size
    return handle_header_block(ctx, header, payload, header.length);
}

int handle_rst_stream_frame(http2_context_t *ctx, frame_header_t header,
                            uint8_t *payload)
{
    (void)payload;
    INFO("<-|%u| RST_STREAM (length: %u, flags: 0x%x, stream_id: %u)", ctx->id,
         header.length, header.flags, header.stream_id);
    // check stream id and stream state
    if (header.stream_id == 0x0) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    // check header length
    if (header.length != 4) {
        http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return -1;
    }

    if (header.stream_id != ctx->last_opened_stream_id) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return -1;
    }

    DEBUG("     - error_code: 0x%x", (unsigned int)bytes_to_uint32(payload));

    // close the stream
    ctx->stream.state = HTTP2_STREAM_CLOSED;

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
        INFO("<-|%u| INVALID HTTP2_PREFACE", ctx->id);
        http2_close_immediate(ctx);
        return 24;
    }

    INFO("<-|%u| HTTP2_PREFACE", ctx->id);

    // update connection state
    ctx->state = HTTP2_WAITING_SETTINGS;

    // prepare settings
    uint32_t settings[] = {
        HTTP2_HEADER_TABLE_SIZE,      HTTP2_ENABLE_PUSH,
        HTTP2_MAX_CONCURRENT_STREAMS, HTTP2_INITIAL_WINDOW_SIZE,
        HTTP2_MAX_FRAME_SIZE,         HTTP2_MAX_HEADER_LIST_SIZE
    };

    // call send_setting_frame
    INFO("->|%u| SETTINGS (length: 36)", ctx->id);
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
    frame_header_t frame_header;
    frame_parse_header(&frame_header, buf, size);

    // check the type
    int frame_size = HTTP2_FRAME_HEADER_SIZE + frame_header.length;
    if (frame_header.type != FRAME_SETTINGS_TYPE ||
        frame_header.flags != FRAME_FLAGS_NONE) {
        http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return frame_size; // discard all input after an error
    }

    // do nothing if the frame has not been fully received
    if (size < frame_size) {
        return 0;
    }

    if (handle_settings_frame(ctx, frame_header,
                              buf + HTTP2_FRAME_HEADER_SIZE) < 0) {
        // if an error ocurred, discard the remaining data
        return size;
    }

    // go to next state
    ctx->state = HTTP2_READY;
    event_read(sock, receiving);

    return frame_size;
}

// Handle read operations while the server is in a ready or closing state
// i.e. after settings exchange
// most frame operations will occur while in this state
int receiving(event_sock_t *sock, int size, uint8_t *buf)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;

    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }

    int bytes_read      = 0;
    int bytes_remaining = size;
    frame_header_t frame_header;

    // process as much data as possible
    while (bytes_remaining > 0) {
        // Wait until frame header is received
        if (bytes_remaining < HTTP2_FRAME_HEADER_SIZE) {
            return bytes_read;
        }

        // read frame
        frame_parse_header(&frame_header, buf + bytes_read,
                           HTTP2_FRAME_HEADER_SIZE);

        // check frame size against local settings
        if (frame_header.length > HTTP2_MAX_FRAME_SIZE) {
            ERROR("cannot process frame (type: 0x%x, length %u) for http/2 "
                  "client %u",
                  frame_header.type, frame_header.length, ctx->id);
            http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
            return bytes_read;
        }

        // we cannot allocate frames larger than the buffer
        // send a FLOW_CONTROL_ERROR
        if (frame_header.length > HTTP2_SOCK_READ_SIZE) {
            ERROR("cannot process frame (type: 0x%x, length %u) for http/2 "
                  "client %u",
                  frame_header.type, frame_header.length, ctx->id);
            http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
            return bytes_read;
        }

        // do nothing if the frame has not been fully received
        if (bytes_remaining < HTTP2_FRAME_HEADER_SIZE + frame_header.length) {
            return bytes_read;
        }

        // update totals
        bytes_read += HTTP2_FRAME_HEADER_SIZE;
        bytes_remaining -= HTTP2_FRAME_HEADER_SIZE;

        // if we have started receiving headers
        // we can only allow CONTINUATION frames for
        // the same stream
        if (ctx->flags & HTTP2_FLAGS_WAITING_END_HEADERS &&
            frame_header.type != FRAME_CONTINUATION_TYPE) {
            http2_error(ctx, HTTP2_PROTOCOL_ERROR);
            return bytes_read;
        }

        int rc = 0;
        switch (frame_header.type) {
            case FRAME_GOAWAY_TYPE:
                rc = handle_goaway_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_SETTINGS_TYPE:
                rc = handle_settings_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_PING_TYPE:
                rc = handle_ping_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_HEADERS_TYPE:
                rc = handle_headers_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_CONTINUATION_TYPE:
                rc = handle_continuation_frame(ctx, frame_header,
                                               buf + bytes_read);
                break;
            case FRAME_WINDOW_UPDATE_TYPE:
                rc = handle_window_update_frame(ctx, frame_header,
                                                buf + bytes_read);
                break;
            case FRAME_RST_STREAM_TYPE:
                rc =
                  handle_rst_stream_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_PRIORITY_TYPE: // ignore priority frames
                INFO("X-|%u| PRIORITY (length: %u, flags: 0x%x, stream_id: %u)",
                     ctx->id, frame_header.length, frame_header.flags,
                     frame_header.stream_id);
                break;
            case FRAME_DATA_TYPE:
                rc = handle_data_frame(ctx, frame_header, buf + bytes_read);
                break;
            case FRAME_PUSH_PROMISE_TYPE:
                INFO("X-|%u| PUSH_PROMISE (length: %u, flags: 0x%x, stream_id: "
                     "%u)",
                     ctx->id, frame_header.length, frame_header.flags,
                     frame_header.stream_id);
                http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                break;
        }

        // update totals
        bytes_read += frame_header.length;
        bytes_remaining -= frame_header.length;

        // if an error ocurred in handling, break and return
        if (rc < 0) {
            break;
        }
    }

    return bytes_read;
}
