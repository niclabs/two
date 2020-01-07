#include <string.h>
#include <assert.h>

#include "frames_v3.h"
#include "http2_v3.h"


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


// default settings from the protocol specification
http2_settings_t default_settings = {
    .header_table_size = 4096,
    .enable_push = 1,
    .max_concurrent_streams = 1,
    .initial_window_size = 65535,
    .max_frame_size = 16384,
    .max_header_list_size = 0xFFFFFFFF
};


// internal send methods
void send_http2_error(http2_context_t *ctx, http2_error_t error);
void send_local_settings(http2_context_t *ctx);


// state methods
int waiting_for_preface(event_sock_t *client, int size, uint8_t *buf);
int waiting_for_settings(event_sock_t *sock, int size, uint8_t *buf);
int ready(event_sock_t *client, int size, uint8_t *buf);
int waiting_to_close(event_sock_t *client, int size, uint8_t *buf);


void http2_init_new_client(event_sock_t *client, http2_context_t *ctx)
{
    assert(client != NULL);
    assert(ctx != NULL);

    client->data = ctx;
    ctx->socket = client;
    ctx->settings = default_settings;
    ctx->stream.id = 2; // server side created streams are even
    ctx->stream.state = STREAM_IDLE;
    ctx->state = HTTP2_WAITING_PREFACE;
    ctx->flags = HTTP2_FLAGS_NONE;

    // TODO: set connection timeout
    event_read_start(client, ctx->read_buf, HTTP2_SOCK_BUF_SIZE, waiting_for_preface);
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
        send_http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return 24;
    }

    // update connection state
    ctx->state = HTTP2_WAITING_SETTINGS;

    // prepare settings
    uint32_t settings [] = { HTTP2_HEADER_TABLE_SIZE, HTTP2_ENABLE_PUSH, \
                             HTTP2_MAX_CONCURRENT_STREAMS, HTTP2_INITIAL_WINDOW_SIZE, \
                             HTTP2_MAX_FRAME_SIZE, HTTP2_MAX_HEADER_LIST_SIZE };

    // call send_setting_frame
    send_settings_frame(client, 0, settings, on_settings_sent);

    // go to next state
    event_read(client, waiting_for_settings);

    return 24;
}

// update remote settings from frame data
void update_remote_settings(http2_context_t *ctx, uint8_t *data, unsigned int size)
{
    http2_settings_t *settings = &ctx->settings;

    // create a local structure to decodify the settings
    struct {
        uint8_t header[9];
        struct {
            uint16_t identifier;
            uint32_t value;
        } __attribute__((packed)) fields[];
    } __attribute__((packed)) frame;
    memcpy(&frame, data, size);

    int len = (size - 9) / 6;
    for (int i = 0; i < len; i++) {
        uint16_t id = frame.fields[i].identifier;
        uint32_t value = frame.fields[i].value;
        switch (id) {
            case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
                settings->header_table_size = value;
                break;
            case HTTP2_SETTINGS_ENABLE_PUSH:
                settings->enable_push = value;
                break;
            case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                settings->max_concurrent_streams = value;
                break;
            case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                if (value > (1U << 31) - 1) {
                    send_http2_error(ctx, HTTP2_FLOW_CONTROL_ERROR);
                    return;
                }
                settings->initial_window_size = value;
                break;
            case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                if (value < (1 << 14) || value > ((1 << 24) - 1)) {
                    send_http2_error(ctx, HTTP2_PROTOCOL_ERROR);
                    return;
                }
                settings->max_frame_size = value;
                break;
            case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                settings->max_header_list_size = value;
                break;
            default:
                break;
        }
    }
}


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
        send_http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return size; // discard all input after an error
    }

    // do nothing if the frame has not been fully received
    if ((unsigned)size < 9 + frame_header.length) {
        return 0;
    }

    // check stream id
    if (frame_header.stream_id != 0x0) {
        send_http2_error(ctx, HTTP2_PROTOCOL_ERROR);
        return size;
    }

    // check frame size
    if (frame_header.length % 6 != 0) {
        send_http2_error(ctx, HTTP2_FRAME_SIZE_ERROR);
        return size;
    }

    // update remote endpoint settings
    update_remote_settings(ctx, buf, size);

    // send settings ack
    send_settings_frame(sock, 1, NULL, close_on_write_error);

    // go to next state
    ctx->state = HTTP2_READY;

    event_read(sock, ready);

    return 0;
}


int ready(event_sock_t *sock, int size, uint8_t *buf)
{
    http2_context_t *ctx = (http2_context_t *)sock->data;
    if (size <= 0) {
        http2_close_immediate(ctx);
        return 0;
    }
}
