#ifndef TWO_CONF_H
#define TWO_CONF_H

#ifdef PROJECT_CONF_H
#include PROJECT_CONF_H
#endif

/**
 * Configure the initial size for the http2 HEADER_TABLE_SIZE
 * in bytes. This value is limited by the HPACK_MAX_DYNAMIC_TABLE_SIZE
 * value.
 *
 * This size increases the total static memory used by http2 client
 * in the implementation.
 *
 * Setting this value to 0 disables the hpack dynamic table
 */
#ifdef CONFIG_HTTP2_HEADER_TABLE_SIZE
#define HTTP2_HEADER_TABLE_SIZE (CONFIG_HTTP2_HEADER_TABLE_SIZE)
#else
#define HTTP2_HEADER_TABLE_SIZE (4096)
#endif

/**
 * Set the initial number of concurrent streams allowed by
 * HTTP2. The current value cannot be larger than 1
 */
#ifdef CONFIG_HTTP2_MAX_CONCURRENT_STREAMS
#define HTTP2_MAX_CONCURRENT_STREAMS (CONFIG_HTTP2_MAX_CONCURRENT_STREAMS)
#else
#define HTTP2_MAX_CONCURRENT_STREAMS (1)
#endif

/**
 * Set the initial value for http2 window size. This value cannot
 * be larger than the HTTP2 read socket size
 */
#ifdef CONFIG_HTTP2_INITIAL_WINDOW_SIZE
#define HTTP2_INITIAL_WINDOW_SIZE (CONFIG_HTTP2_INITIAL_WINDOW_SIZE)
#else
#define HTTP2_INITIAL_WINDOW_SIZE (512)
#endif

/**
 * Set the initial value for http2 settings MAX_FRAME_SIZE
 */
#ifdef CONFIG_HTTP2_MAX_FRAME_SIZE
#define HTTP2_MAX_FRAME_SIZE (CONFIG_HTTP2_MAX_FRAME_SIZE)
#else
#define HTTP2_MAX_FRAME_SIZE (16384)
#endif

/**
 * Set the maximum size in bytes for the received
 * header list. This memory is reserved in the stack
 * and it does not increase the size of the static memory
 * used by the implementation
 */
#ifdef CONFIG_HTTP2_MAX_HEADER_LIST_SIZE
#define HTTP2_MAX_HEADER_LIST_SIZE (CONFIG_HTTP2_MAX_HEADER_LIST_SIZE)
#else
#define HTTP2_MAX_HEADER_LIST_SIZE (512)
#endif

/**
 * Set the maximum time in milliseconds to wait for the remote endpoint
 * to reply to a settings frame
 */
#ifdef CONFIG_HTTP2_SETTINGS_WAIT
#define HTTP2_SETTINGS_WAIT (CONFIG_HTTP2_SETTINGS_WAIT)
#else
#define HTTP2_SETTINGS_WAIT (300)
#endif

/**
 * Set the size for the read socket buffer. This effectively
 * limits the maximum frame size that can be received by the
 * remote endpoint. This value must be larger than the
 * HTTP2_INITIAL_WINDOW_SIZE.
 *
 * Changes in this value alter the total static memory used
 * by the implementation.
 */
#ifdef CONFIG_HTTP2_SOCK_READ_SIZE
#define HTTP2_SOCK_READ_SIZE (CONFIG_HTTP2_SOCK_READ_SIZE)
#else
#define HTTP2_SOCK_READ_SIZE (HTTP2_INITIAL_WINDOW_SIZE)
#endif

/**
 * Set the size for the write socket buffer. This effectively limits
 * the maximum size of frame that can be sent.
 *
 * Changes in this value alter the total static memory used
 * by the implementation.
 */
#ifdef CONFIG_HTTP2_SOCK_WRITE_SIZE
#define HTTP2_SOCK_WRITE_SIZE (CONFIG_HTTP2_SOCK_WRITE_SIZE)
#else
#define HTTP2_SOCK_WRITE_SIZE (512)
#endif

/**
 * Set the maximum total data that can be received by
 * a stream. This means, the total header block size that
 * can be received in a single http2 stream, or the maximum total
 * data that can be sent in a http response.
 *
 * Changes in this value alter the total static memory used
 * by the implementation.
 */
#ifdef CONFIG_HTTP2_STREAM_BUF_SIZE
#define HTTP2_STREAM_BUF_SIZE (CONFIG_HTTP2_STREAM_BUF_SIZE)
#else
#define HTTP2_STREAM_BUF_SIZE (512)
#endif

/**
 * Set the maximum number of clients allowed by the server
 *
 * This is effectively a multiplier for the total static
 * memory used by the implementation
 */
#ifdef CONFIG_HTTP2_MAX_CLIENTS
#define EVENT_MAX_SOCKETS ((CONFIG_HTTP2_MAX_CLIENTS) + 1)
#else
#define EVENT_MAX_SOCKETS (3)
#endif

/**
 * Set the maximum number of resource paths allowed by
 * http servers.
 *
 * Changes in this value alter the total static memory used
 * by the implementation.
 */
#ifdef CONFIG_TWO_MAX_RESOURCES
#define TWO_MAX_RESOURCES (CONFIG_TWO_MAX_RESOURCES)
#else
#define TWO_MAX_RESOURCES (4)
#endif

/**
 * Event module log level (off by default)
 */
#ifdef CONFIG_LOG_LEVEL_EVENT
#define LOG_LEVEL_EVENT (CONFIG_LOG_LEVEL_EVENT)
#else
#define LOG_LEVEL_EVENT (LOG_LEVEL_OFF)
#endif

/**
 * HPACK module log level (info by default)
 */
#ifdef CONFIG_LOG_LEVEL_HPACK
#define LOG_LEVEL_HPACK (CONFIG_LOG_LEVEL_HPACK)
#else
#define LOG_LEVEL_HPACK (LOG_LEVEL_OFF)
#endif

/**
 * HTTP module log level (info by default)
 */
#ifdef CONFIG_LOG_LEVEL_HTTP
#define LOG_LEVEL_HTTP (CONFIG_LOG_LEVEL_HTTP)
#else
#define LOG_LEVEL_HTTP (LOG_LEVEL_INFO)
#endif

/**
 * HTTP2 module log level (info by default)
 */
#ifdef CONFIG_LOG_LEVEL_HTTP2
#define LOG_LEVEL_HTTP2 (CONFIG_LOG_LEVEL_HTTP2)
#else
#define LOG_LEVEL_HTTP2 (LOG_LEVEL_INFO)
#endif

#endif
