#ifndef HTTP2_V3_H
#define HTTP2_V3_H

#include <stdint.h>
#include "event.h"

/**
 * SETTINGS_HEADER_TABLE_SIZE
 *
 * Allows the sender to inform the remote endpoint of the maximum size of the
 * header compression table used to decode header blocks, in octets. The encoder
 * can select any size equal to or less than this value by using signaling
 * specific to the header compression format inside a header block.
 * The initial value is 4,096 octets.
 *
 * The macro CONFIG_HTTP2_HEADER_TABLE_SIZE sets the local default for the
 * HEADER_TABLE_SIZE.
 */
#ifndef CONFIG_HTTP2_HEADER_TABLE_SIZE
#define HTTP2_HEADER_TABLE_SIZE (4096)
#else
#define HTTP2_HEADER_TABLE_SIZE (CONFIG_HTTP2_HEADER_TABLE_SIZE)
#endif

/**
 * SETTINGS_ENABLE_PUSH
 *
 * This setting can be used to disable server push (Section 8.2).
 * An endpoint MUST NOT send a PUSH_PROMISE frame if it receives this parameter set
 * to a value of 0. An endpoint that has both set this parameter to 0 and had it
 * acknowledged MUST treat the receipt of a PUSH_PROMISE frame as a connection error
 * (Section 5.4.1) of type PROTOCOL_ERROR.
 *
 * Server push is not supported by the current implementation
 */
#ifndef CONFIG_HTTP2_ENABLE_PUSH
#define HTTP2_ENABLE_PUSH (0)
#elif CONFIG_HTTP2_ENABLE_PUSH != 0
#error "Server push is not implemented."
#endif

/**
 * SETTINGS_MAX_CONCURRENT_STREAMS
 *
 * Indicates the maximum number of concurrent streams that the sender will
 * allow. This limit is directional: it applies to the number of streams that
 * the sender permits the receiver to create.
 *
 * The server does not provide support for multiple streams.
 */
#ifndef CONFIG_HTTP2_MAX_CONCURRENT_STREAMS
#define HTTP2_MAX_CONCURRENT_STREAMS (1)
#elif CONFIG_HTTP2_MAX_CONCURRENT_STREAMS > 1
#error "Support for multiple concurrent streams is not implemented."
#endif

/**
 * Indicates the sender's initial window size (in octets) for stream-level
 * flow control. The initial value is 216-1 (65,535) octets.
 *
 * This setting affects the window size of all streams (see Section 6.9.2).
 *
 * The macro CONFIG_HTTP2_INITIAL_WINDOW_SIZE sets the local default for the
 * INITIAL_WINDOW_SIZE.
 */
#ifndef CONFIG_HTTP2_INITIAL_WINDOW_SIZE
#define HTTP2_INITIAL_WINDOW_SIZE (65535)
#elif CONFIG_HTTP2_INITIAL_WINDOW_SIZE > ((1 << 31) - 1)
#error "Settings initial window size cannot be larger than 2^31 - 1."
#else
#define HTTP2_INITIAL_WINDOW_SIZE (CONFIG_HTTP2_INITIAL_WINDOW_SIZE)
#endif

/**
 * Indicates the size of the largest frame payload that the sender is willing
 * to receive, in octets.
 *
 * The initial value is 214 (16,384) octets. The value advertised by an endpoint
 * MUST be between this initial value and the maximum allowed frame size
 * (2^24-1 or 16,777,215 octets), inclusive. Values outside this range MUST
 * be treated as a connection error (Section 5.4.1) of type PROTOCOL_ERROR.
 *
 * The macro CONFIG_HTTP2_MAX_FRAME_SIZE sets the local default for MAX_FRAME_SIZE.
 */
#ifndef CONFIG_HTTP2_MAX_FRAME_SIZE
#define HTTP2_MAX_FRAME_SIZE (16384)
#elif CONFIG_HTTP2_MAX_FRAME_SIZE < (1 << 14) || CONFIG_HTTP2_MAX_FRAME_SIZE > ((1 << 24) - 1)
#error "Settings max frame size can only be a number between 2^14 and 2^24 - 1."
#else
#define HTTP2_MAX_FRAME_SIZE (CONFIG_HTTP2_MAX_FRAME_SIZE)
#endif

/**
 * This advisory setting informs a peer of the maximum size of header list
 * that the sender is prepared to accept, in octets. The value is based on
 * the uncompressed size of header fields, including the length of the name and value in
 * octets plus an overhead of 32 octets for each header field.
 *
 * The macro CONFIG_HTTP2_MAX_HEADER_LIST_SIZE sets the local default for
 * MAX_HEADER_LIST_SIZE.
 */
#ifndef CONFIG_HTTP2_MAX_HEADER_LIST_SIZE
#define HTTP2_MAX_HEADER_LIST_SIZE (0xFFFFFFFF)
#else
#define HTTP2_MAX_HEADER_LIST_SIZE (CONFIG_MAX_HEADER_LIST_SIZE)
#endif

/**
 * The macro CONFIG_HTTP2_SOCK_BUF_SIZE sets the maximum size of the
 * read buffer for incoming connections. This means that at most
 * CONFIG_HTTP2_SOCK_BUF_SIZE bytes can be pending on a given connection
 * while a frame is being procesed by the library.
 */
#if !defined(CONFIG_HTTP2_SOCK_BUF_SIZE) && defined(CONTIKI)
#define HTTP2_SOCK_BUF_SIZE (UIP_TCP_MSS) // maximum segment size is 1280 by default in contiki
#elif !defined(CONFIG_HTTP2_SOCK_BUF_SIZE)
#define HTTP2_SOCK_BUF_SIZE (4096)
#else
#define HTTP2_SOCK_BUF_SIZE (CONFIG_HTTP2_SOCK_BUF_SIZE)
#endif


typedef enum {
    HTTP2_NO_ERROR              = 0x0,
    HTTP2_PROTOCOL_ERROR        = 0x1,
    HTTP2_INTERNAL_ERROR        = 0x2,
    HTTP2_FLOW_CONTROL_ERROR    = 0x3,
    HTTP2_SETTINGS_TIMEOUT      = 0x4,
    HTTP2_STREAM_CLOSED         = 0x5,
    HTTP2_FRAME_SIZE_ERROR      = 0x6,
    HTTP2_REFUSED_STREAM        = 0x7,
    HTTP2_CANCEL                = 0x8,
    HTTP2_COMPRESSION_ERROR     = 0x9,
    HTTP2_CONNECT_ERROR         = 0xa,
    HTTP2_ENHANCE_YOUR_CALM     = 0xb,
    HTTP2_INADEQUATE_SECURITY   = 0xc,
    HTTP2_HTTP_1_1_REQUIRED     = 0xd
} http2_error_t;

typedef struct http2_stream {
    uint32_t id;
    enum {
        STREAM_IDLE,
        STREAM_OPEN,
        STREAM_HALF_CLOSED_LOCAL,
        STREAM_HALF_CLOSED_REMOTE,
        STREAM_CLOSED
    } state;
} http2_stream_t;

typedef struct http2_settings {
    uint32_t header_table_size;
    uint32_t enable_push;
    uint32_t max_concurrent_streams;
    uint32_t initial_window_size;
    uint32_t max_frame_size;
    uint32_t max_header_list_size;
} http2_settings_t;

typedef struct http2_context {
    // make http2_context a linked lists
    // IMPORTANT: this pointer must be the first of the struct
    struct http2_context *next;

    // tcp socket
    event_sock_t *socket;

    // remote end settings
    http2_settings_t settings;

    // local state
    enum {
        HTTP2_CLOSED,
        HTTP2_WAITING_PREFACE,
        HTTP2_WAITING_SETTINGS,
        HTTP2_READY,
        HTTP2_GOAWAY
    } state;

    // current stream
    http2_stream_t stream;

    // sock read buffer
    uint8_t read_buf[HTTP2_SOCK_BUF_SIZE];
} http2_context_t;


void http2_init_new_client(event_sock_t *client, http2_context_t *ctx);
void http2_close_gracefully(http2_context_t *ctx);
void http2_close_immediate(http2_context_t *ctx);

#endif
