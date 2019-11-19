#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <signal.h>


#include "event.h"

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_HEADER_SIZE 9
#define HTTP2_MAX_FRAME_SIZE 4096

// settings
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE (1)
#define HTTP2_SETTINGS_ENABLE_PUSH (2)
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS (3)
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE (4)
#define HTTP2_SETTINGS_MAX_FRAME_SIZE (5)
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE (6)

#define MIN(x, y) ((x) < (y) ? (x) : (y))

// booleans
typedef int bool;
#define true 1
#define false 0

// local struct definitions
typedef struct frame_header {
    uint32_t length; // 24 bits
    uint8_t type;
    uint8_t flags;
    uint32_t stream_id; // 31 bits
} frame_header_t;


typedef enum {
    SETTINGS_FRAME = (uint8_t) 0x4
} frame_type_t;

typedef struct raw_frame_header {
    uint8_t data[HTTP2_HEADER_SIZE];
} __attribute__((packed)) raw_frame_header_t;

typedef struct frame_settings {
    struct raw_frame_header header;
    struct frame_settings_field {
        uint16_t identifier;
        uint32_t value;
    } __attribute__((packed)) fields[];
} __attribute__((packed)) frame_settings_t;

typedef struct http2_settings {
    uint32_t header_table_size;
    uint32_t enable_push;
    uint32_t max_concurrent_streams;
    uint32_t initial_window_size;
    uint32_t max_frame_size;
    uint32_t max_header_list_size;
} http2_settings_t;

typedef struct http2_ctx {
    event_sock_t *sock;

    // Current header data
    frame_header_t header;
    uint8_t frame[HTTP2_MAX_FRAME_SIZE];

    // Client state
    enum {
        HTTP2_IDLE,
        HTTP2_WAITING_PREFACE,
        HTTP2_WAITING_SETTINGS
    } state;

    // Client settings
    http2_settings_t settings;

    struct http2_ctx *next;
} http2_ctx_t;


// default application settings
http2_settings_t default_settings = {
    .header_table_size = 4096,
    .enable_push = 0,
    .max_concurrent_streams = 1,
    .initial_window_size = 65535,
    .max_frame_size = 16384,
    .max_header_list_size = 0xFFFFFFFF
};

// static variables
static event_loop_t loop;
static event_sock_t *server;

static http2_ctx_t http2_ctx_list[EVENT_MAX_HANDLES];
static http2_ctx_t *connected;
static http2_ctx_t *unused;

// methods
http2_ctx_t *get_unused_ctx()
{
    if (unused == NULL) {
        return NULL;
    }

    http2_ctx_t *ctx = unused;
    unused = ctx->next;

    ctx->next = connected;
    connected = ctx;

    return ctx;
}

void free_ctx(http2_ctx_t *ctx)
{
    http2_ctx_t *curr = connected, *prev = NULL;

    while (curr != NULL) {
        if (curr == ctx) {
            if (prev == NULL) {
                connected = curr->next;
            }
            else {
                prev = prev->next;
            }

            curr->next = unused;
            unused = curr;

            return;
        }

        prev = curr;
        curr = curr->next;
    }

}

void on_server_close(event_sock_t *handle)
{
    (void)handle;
    INFO("Server closed");
    exit(0);
}

void close_server(int sig)
{
    (void)sig;
    event_close(server, on_server_close);
}

void on_client_close(event_sock_t *handle)
{
    if (handle->data != NULL) {
        http2_ctx_t *ctx = (http2_ctx_t *)handle->data;
        free_ctx(ctx);
    }

    INFO("Client closed");
}

void parse_frame_header(uint8_t *data, frame_header_t *header)
{
    memset(header, 0, sizeof(frame_header_t));

    // read header length
    header->length |= (data[2]);
    header->length |= (data[1] << 8);
    header->length |= (data[0] << 16);

    // read type and flags
    header->type = data[3];
    header->flags = data[4];

    // unset the first bit of the id
    header->stream_id |= (data[5] & 0x7F) << 24;
    header->stream_id |= data[6] << 16;
    header->stream_id |= data[7] << 8;
    header->stream_id |= data[8] << 0;

}

void create_frame_header(raw_frame_header_t *header, uint32_t length, uint8_t flags, uint8_t type, uint32_t stream_id)
{
    header->data[0] = (length >> 16) & 0xFF;
    header->data[1] = (length >> 8) & 0xFF;
    header->data[2] = length & 0xFF;
    header->data[3] = type;
    header->data[4] = flags;
    header->data[5] = (stream_id >> 24) & 0x7F;
    header->data[6] = (stream_id >> 16) & 0xFF;
    header->data[7] = (stream_id >> 8) & 0xFF;
    header->data[8] = stream_id & 0xFF;
}

void send_settings_frame(http2_ctx_t *ctx, struct frame_settings_field *fields, int len, bool ack, event_write_cb on_send)
{
    assert(ctx->sock != NULL);
    assert(fields == NULL || ack == false);

    if (ack) {
        raw_frame_header_t hd;
        create_frame_header(&hd, 0, ack, SETTINGS_FRAME, 0);
        event_write(ctx->sock, HTTP2_HEADER_SIZE, hd.data, on_send);
    }
    else {
        int size = len * sizeof(struct frame_settings_field);

        raw_frame_header_t hd;
        create_frame_header(&hd, size, 0, SETTINGS_FRAME, 0);

        uint8_t buf[HTTP2_HEADER_SIZE + size];
        memcpy(buf, hd.data, HTTP2_HEADER_SIZE);
        memcpy(buf + HTTP2_HEADER_SIZE, &fields, size - HTTP2_HEADER_SIZE);

        event_write(ctx->sock, size, buf, on_send);
    }
}


void on_settings_ack_sent(event_sock_t *client, int status)
{
    if (status < 0) {
        ERROR("Could not send settings ack");
        event_close(client, on_client_close);
        return;
    }

    DEBUG("settings ACK sent");
}


int read_settings_payload(event_sock_t *client, ssize_t size, uint8_t *buf)
{
    // read context from client data
    assert(client->data != NULL);
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;

    if (size >= (signed)ctx->header.length) {
        // copy payload to frame
        memcpy(ctx->frame + HTTP2_HEADER_SIZE, buf, ctx->header.length);

        // cast the frame to settings
        frame_settings_t *settings = (frame_settings_t *)ctx->frame;

        int len = ctx->header.length / sizeof(struct frame_settings_field);
        for (int i = 0; i < len; i++) {
            uint16_t id = settings->fields[i].identifier;
            uint32_t value = settings->fields[i].value;
            switch (id) {
                case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
                    ctx->settings.header_table_size = value;
                    break;
                case HTTP2_SETTINGS_ENABLE_PUSH:
                    if (value != 0) {
                        // todo: send protocol error
                        return -1;
                    }
                    ctx->settings.enable_push = 0;
                    break;
                case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                    if (value > 1) {
                        // todo: send internal error
                        return -1;
                    }
                    ctx->settings.max_concurrent_streams = value;
                    break;
                case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                    if (value > (1U << 31) - 1) {
                        // todo: send flow control error error
                        return -1;
                    }
                    ctx->settings.initial_window_size = value;
                    break;
                case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                    if (value < (1 << 14) || value > ((1 << 24) - 1)) {
                        // todo: send protocol error
                        return -1;
                    }
                    ctx->settings.max_frame_size = value;
                    break;
                case HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE:
                    ctx->settings.max_header_list_size = value;
                    break;
                default:
                    // should i error here?
                    break;
            }
        }

        // send settings ack
        send_settings_frame(ctx, NULL, 0, true, on_settings_ack_sent);

        return ctx->header.length;
    }
    else if (size < 0) {
        INFO("Remote client closed");
        event_close(client, on_client_close);
    }
    return 0;
}

int process_header(event_sock_t *client, http2_ctx_t *ctx, frame_header_t *header)
{
    switch (ctx->state) {
        case HTTP2_WAITING_SETTINGS:
            if (header->type == SETTINGS_FRAME) {
                // wait for payload
                DEBUG("waiting for SETTINGS frame payload");
                event_read(client, read_settings_payload);
            }
            break;
        default:
            ERROR("Unexpected state");
            return -1;
    }
    return 0;
}

int read_header(event_sock_t *client, ssize_t size, uint8_t *buf)
{
    if (size >= HTTP2_HEADER_SIZE) {
        // get client context from socket data
        assert(client->data != NULL);
        http2_ctx_t *ctx = (http2_ctx_t *)client->data;

        // Copy header data to current frame
        memcpy(ctx->frame, buf, HTTP2_HEADER_SIZE);

        frame_header_t *header = &ctx->header;
        parse_frame_header(ctx->frame, header);

        DEBUG("received header = {length: %d, type: %d, flags: %d, stream_id: %d}", \
          header->length, header->type, header->flags, header->stream_id);

        event_read_stop(client);
        if (process_header(client, ctx, header) < 0) {
            event_close(client, on_client_close);
        }

        return HTTP2_HEADER_SIZE;
    }
    else if (size < 0) {
        INFO("Remote client closed");
        event_close(client, on_client_close);
    }
    return 0;
}


void on_settings_sent(event_sock_t *client, int status)
{
    if (status < 0) {
        ERROR("Failed to send local SETTINGS");
        event_close(client, on_client_close);
        return;
    }
    DEBUG("local SETTINGS sent");

    assert(client->data != NULL);
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;

    // configure client state to wait for settings
    ctx->state = HTTP2_WAITING_SETTINGS;

    // read frame header
    event_read(client, read_header);
}

int read_preface(event_sock_t *client, ssize_t size, uint8_t *buf)
{
    if (size >= 24) {
        buf[size] = '\0';
        if (strncmp(HTTP2_PREFACE, (const char *)buf, 24) != 0) {
            ERROR("Bad preface received");
            event_close(client, on_client_close);

            return 0;
        }

        DEBUG("received HTTP/2 preface");
        assert(client->data != NULL);
        http2_ctx_t *ctx = (http2_ctx_t *)client->data;

        // configure frame settings
        struct frame_settings_field fields[5];
        for (int i = 0; i < 5; i++) {
            fields[i].identifier = i + 1;
            switch (fields[i].identifier) {
                case HTTP2_SETTINGS_HEADER_TABLE_SIZE:
                    fields[i].value = default_settings.header_table_size;
                    break;
                case HTTP2_SETTINGS_ENABLE_PUSH:
                    fields[i].value = default_settings.enable_push;
                    break;
                case HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS:
                    fields[i].value = default_settings.max_concurrent_streams;
                    break;
                case HTTP2_SETTINGS_INITIAL_WINDOW_SIZE:
                    fields[i].value = default_settings.initial_window_size;
                    break;
                case HTTP2_SETTINGS_MAX_FRAME_SIZE:
                    fields[i].value = default_settings.max_frame_size;
                    break;
                default:
                    break;
            }

        }

        // send settings
        send_settings_frame(ctx, fields, 5, false, on_settings_sent);

        return 24;
    }
    else if (size <= 0) {
        INFO("Remote client closed");
        event_close(client, on_client_close);
    }
    return 0;
}

void on_new_connection(event_sock_t *server, int status)
{
    if (status < 0) {
        ERROR("Connection error");
        // error!
        return;
    }

    INFO("New client connection");
    event_sock_t *client = event_sock_create(server->loop);
    if (event_accept(server, client) == 0) {
        http2_ctx_t *ctx = get_unused_ctx();
        assert(ctx != NULL); // if this happens there is an implementation error

        // set client state to waiting preface
        ctx->state = HTTP2_WAITING_PREFACE;
        ctx->sock = client;

        // reference http2_ctx in socket data
        client->data = ctx;
        ctx->settings = default_settings;

        event_read(client, read_preface);
    }
    else {
        event_close(client, on_client_close);
    }
}

int main()
{
    // set client memory
    memset(http2_ctx_list, 0, EVENT_MAX_HANDLES * sizeof(http2_ctx_t));
    for (int i = 0; i < EVENT_MAX_HANDLES - 1; i++) {
        http2_ctx_list[i].next = &http2_ctx_list[i + 1];
    }
    connected = NULL;
    unused = &http2_ctx_list[0];

    event_loop_init(&loop);
    server = event_sock_create(&loop);

    signal(SIGINT, close_server);

    int r = event_listen(server, 8888, on_new_connection);
    if (r < 0) {
        ERROR("Could not start server");
        return 1;
    }
    event_loop(&loop);
}
