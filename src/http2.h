#ifndef HTTP2_V3_H
#define HTTP2_V3_H

#include <stdint.h>
#include "event.h"
#include "header_list.h"
#include "hpack/hpack.h"

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
#define HTTP2_HEADER_TABLE_SIZE (HPACK_MAX_DYNAMIC_TABLE_SIZE)
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
#define HTTP2_INITIAL_WINDOW_SIZE (1024)
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
#define HTTP2_MAX_HEADER_LIST_SIZE (HEADER_LIST_MAX_SIZE)

/**
 * The macro CONFIG_HTTP2_SOCK_READ_SIZE sets the maximum size of the
 * read buffer for incoming connections. This means that at most
 * CONFIG_HTTP2_SOCK_READ_SIZE bytes can be pending on a given connection
 * while a frame is being procesed by the library.
 */
#if !defined(CONFIG_HTTP2_SOCK_READ_SIZE) && defined(CONTIKI)
#define HTTP2_SOCK_READ_SIZE (1024) // maximum segment size is 1280 by default in contiki
#elif !defined(CONFIG_HTTP2_SOCK_READ_SIZE)
#define HTTP2_SOCK_READ_SIZE (4096)
#else
#define HTTP2_SOCK_READ_SIZE (CONFIG_HTTP2_SOCK_READ_SIZE)
#endif

/**
 * The stream buffer size is the maximum header block size that can be received
 * or the maximum total data length that can be sent.
 *
 * Its ideal size is difficult to calculate, but it is expected that
 * sum(header_block_size) < max_header_list_size
 * (TODO: study compression rates).
 */
#ifndef CONTIG_HTTP2_STREAM_BUF_SIZE
#define HTTP2_STREAM_BUF_SIZE (512)
#elif CONFIG_HTTP2_STREAM_BUF_SIZE > ((1 << 16) - 1)
#error "Stream buffer size can be at most a 16-bit unsigned integer by implementation."
#else
#define HTTP2_STREAM_BUF_SIZE (CONFIG_HTTP2_STREAM_BUF_SIZE)
#endif

// Verify correct buffer sizes
#if HTTP2_SOCK_READ_SIZE < HTTP2_INITIAL_WINDOW_SIZE
#error "The implementation does not allow to receive more bytes than the allocated read buffer. Either increase the value of HTTP2_SOCK_READ_SIZE or reduce the value of HTTP2_INITIAL_WINDOW_SIZE"
#endif

#if defined(CONTIKI) && HTTP2_SOCK_READ_SIZE > UIP_TCP_MSS
#error "The socket read size cannot be larger than the maximum TCP segment size"
#endif

typedef enum {
    HTTP2_NO_ERROR              = (uint8_t) 0x0,
    HTTP2_PROTOCOL_ERROR        = (uint8_t) 0x1,
    HTTP2_INTERNAL_ERROR        = (uint8_t) 0x2,
    HTTP2_FLOW_CONTROL_ERROR    = (uint8_t) 0x3,
    HTTP2_SETTINGS_TIMEOUT      = (uint8_t) 0x4,
    HTTP2_STREAM_CLOSED_ERROR   = (uint8_t) 0x5,
    HTTP2_FRAME_SIZE_ERROR      = (uint8_t) 0x6,
    HTTP2_REFUSED_STREAM        = (uint8_t) 0x7,
    HTTP2_CANCEL                = (uint8_t) 0x8,
    HTTP2_COMPRESSION_ERROR     = (uint8_t) 0x9,
    HTTP2_CONNECT_ERROR         = (uint8_t) 0xa,
    HTTP2_ENHANCE_YOUR_CALM     = (uint8_t) 0xb,
    HTTP2_INADEQUATE_SECURITY   = (uint8_t) 0xc,
    HTTP2_HTTP_1_1_REQUIRED     = (uint8_t) 0xd
} http2_error_t;

typedef struct http2_stream {
    uint32_t id;
    enum {
        HTTP2_STREAM_IDLE               = (uint8_t) 0x0,
        HTTP2_STREAM_OPEN               = (uint8_t) 0x1,
        HTTP2_STREAM_HALF_CLOSED_LOCAL  = (uint8_t) 0x2,
        HTTP2_STREAM_HALF_CLOSED_REMOTE = (uint8_t) 0x3,

        // states idle and closed are equal in our implementation
        HTTP2_STREAM_CLOSED             = (uint8_t) 0x0
    } state;

    // stream window size has by default
    // the same value as the connection
    // window size
    int32_t window_size;

    // header block receiving buffer
    // and data output buffer
    uint8_t buf[HTTP2_STREAM_BUF_SIZE];
    uint16_t buflen;
    uint8_t *bufptr;
} __attribute__((packed)) http2_stream_t;

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
        HTTP2_CLOSED            = (uint8_t) 0x0,
        HTTP2_WAITING_PREFACE   = (uint8_t) 0x1,
        HTTP2_WAITING_SETTINGS  = (uint8_t) 0x2,
        HTTP2_READY             = (uint8_t) 0x3,
        HTTP2_CLOSING           = (uint8_t) 0x4
    } state;

    // connection window size
    // for the remote endpoint
    // since we will ignore DATA frames
    // we don not require to maintain
    // a local window size
    int32_t window_size;

    // current stream
    http2_stream_t stream;

    // last opened stream
    uint32_t last_opened_stream_id;

    // http2 state flags
    uint8_t flags;

    // sock read buffer
    uint8_t read_buf[HTTP2_SOCK_READ_SIZE];

    // hpack dynamic table
    hpack_dynamic_table_t hpack_dynamic_table;
} __attribute__((packed)) http2_context_t;


http2_context_t *http2_new_client(event_sock_t *client);
int http2_close_gracefully(http2_context_t *ctx);
void http2_close_immediate(http2_context_t *ctx);
void http2_error(http2_context_t *ctx, http2_error_t error);

#endif
