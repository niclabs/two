#include <string.h>

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
#include <stdlib.h>
#include <signal.h>
#endif

#ifndef CONTIKI
#include <assert.h>
#else
#include "contiki.h"
#include "lib/assert.h"
#endif

#include "event.h"

#if TLS_ENABLE
#include <bearssl.h>
#endif

#define LOG_LEVEL LOG_LEVEL_DEBUG
#include "logging.h"

#define TINY_MAX_CLIENTS (EVENT_MAX_SOCKETS - 1)

#define HTTP2_PREFACE "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n"
#define HTTP2_HEADER_SIZE 9

#undef HTTP2_MAX_FRAME_SIZE
#define HTTP2_MAX_FRAME_SIZE 2048

// settings
#define HTTP2_SETTINGS_HEADER_TABLE_SIZE (1)
#define HTTP2_SETTINGS_ENABLE_PUSH (2)
#define HTTP2_SETTINGS_MAX_CONCURRENT_STREAMS (3)
#define HTTP2_SETTINGS_INITIAL_WINDOW_SIZE (4)
#define HTTP2_SETTINGS_MAX_FRAME_SIZE (5)
#define HTTP2_SETTINGS_MAX_HEADER_LIST_SIZE (6)

#ifndef MIN
#define MIN(x, y) ((x) < (y) ? (x) : (y))
#endif

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
        HTTP2_WAITING_SETTINGS,
        HTTP2_WAITING_SETTINGS_PAYLOAD,
    } state;
#if TLS_ENABLE
    br_ssl_server_context sc;
    uint8_t handshake;
    unsigned char iobuf_in[BR_SSL_BUFSIZE_INPUT];
    unsigned char iobuf_out[BR_SSL_BUFSIZE_OUTPUT];
#endif
    // Client settings
    http2_settings_t settings;

    struct http2_ctx *next;

    uint8_t event_buf[HTTP2_MAX_FRAME_SIZE];
    uint8_t event_wbuf[HTTP2_MAX_FRAME_SIZE];
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

static http2_ctx_t http2_ctx_list[TINY_MAX_CLIENTS];
static http2_ctx_t *connected;
static http2_ctx_t *unused;

#if TLS_ENABLE
static const unsigned char RSA_P[] = {
        0xE9, 0x1F, 0xBC, 0xC1, 0x84, 0x19, 0x00, 0x1B, 0x87, 0xC2, 0x10, 0xF6,
        0x04, 0xF8, 0xF9, 0x24, 0xBD, 0xA7, 0x8F, 0xB6, 0xCB, 0xB1, 0xA5, 0x8A,
        0xB9, 0x4F, 0x44, 0x18, 0xB5, 0x1B, 0xBC, 0x4B, 0x48, 0x90, 0x40, 0x43,
        0xCB, 0x3A, 0x75, 0x29, 0x04, 0x9D, 0xFF, 0x6F, 0xA4, 0x48, 0xF1, 0x86,
        0x6C, 0xA0, 0xF6, 0xFB, 0xF7, 0x79, 0xFF, 0x26, 0xC0, 0x03, 0x15, 0x31,
        0xB3, 0xB5, 0x15, 0xEE, 0x46, 0xCD, 0xD0, 0x99, 0xDE, 0xFC, 0x1F, 0xA0,
        0x56, 0xC4, 0x9C, 0x6C, 0x77, 0x2C, 0x68, 0x3B, 0xD9, 0xA0, 0xEB, 0x7A,
        0xF2, 0xF5, 0xB1, 0x4B, 0x45, 0xEE, 0x8E, 0xB2, 0x43, 0xB5, 0x2A, 0x32,
        0x18, 0x95, 0xEC, 0x22, 0x1D, 0x4F, 0x5E, 0x80, 0xBE, 0xF0, 0x6B, 0xEF,
        0x6E, 0x24, 0xEE, 0xC0, 0x76, 0x34, 0xD2, 0xA7, 0x0F, 0xF9, 0x4C, 0xEA,
        0x43, 0xAB, 0xA1, 0x8A, 0x8F, 0x2B, 0x52, 0x97
};

static const unsigned char RSA_Q[] = {
        0xE4, 0x0D, 0x6D, 0x85, 0xF1, 0xDA, 0x93, 0x3D, 0xAC, 0x75, 0x6D, 0xA7,
        0x7C, 0x21, 0x4D, 0x83, 0x82, 0x90, 0xF4, 0xC4, 0x59, 0x30, 0x2B, 0x24,
        0xFE, 0xE6, 0x70, 0xEF, 0x2C, 0x5A, 0xD9, 0xC7, 0xD2, 0x71, 0xCC, 0x76,
        0x02, 0x33, 0x8A, 0xAD, 0x2F, 0xC6, 0x50, 0x05, 0x16, 0x15, 0x2B, 0xA5,
        0x6C, 0x07, 0x06, 0x26, 0xA0, 0xCC, 0x92, 0xBB, 0x15, 0x38, 0x9A, 0x37,
        0x78, 0x13, 0x17, 0x4B, 0x95, 0xB6, 0xAD, 0x8F, 0xB5, 0x80, 0xAE, 0x34,
        0x69, 0x22, 0xA6, 0x2B, 0xB9, 0x80, 0x6C, 0xDD, 0x01, 0x8D, 0x2E, 0x67,
        0x90, 0x7A, 0xD0, 0xA1, 0x35, 0xB0, 0xC1, 0xE4, 0x3B, 0x14, 0xD4, 0x13,
        0x5E, 0xEB, 0x40, 0xC9, 0x66, 0x60, 0xF1, 0x1F, 0xFE, 0x40, 0x2D, 0x8F,
        0x6E, 0x89, 0x43, 0xB8, 0x23, 0xAE, 0x76, 0xA3, 0x2C, 0x3D, 0x6B, 0xCF,
        0xB5, 0x77, 0xB4, 0xAE, 0x3A, 0xC0, 0x0B, 0xF3
};

static const unsigned char RSA_DP[] = {
        0xA8, 0x47, 0x03, 0x8E, 0xC0, 0xD6, 0xF7, 0x0F, 0xE8, 0x58, 0x3A, 0xBC,
        0x0B, 0xEC, 0xD8, 0x93, 0x1F, 0xDF, 0xB3, 0x4A, 0xA5, 0x10, 0x8F, 0xC9,
        0x6A, 0x68, 0x80, 0x64, 0x41, 0x5F, 0x4A, 0xF5, 0x20, 0xE5, 0x17, 0xAE,
        0x98, 0x25, 0x93, 0x6A, 0xCF, 0x6D, 0x69, 0x74, 0x62, 0x27, 0x51, 0x48,
        0xD2, 0x63, 0x02, 0xC5, 0xF0, 0xE6, 0xFC, 0x3A, 0x31, 0x82, 0x48, 0x2B,
        0x3F, 0x68, 0x68, 0xF3, 0x3D, 0xE2, 0xD5, 0x40, 0x2D, 0x08, 0xDB, 0x9F,
        0x76, 0xE2, 0xA7, 0x73, 0x58, 0x37, 0x12, 0xEA, 0x98, 0xF6, 0xA2, 0xE4,
        0x76, 0x3A, 0xCA, 0x06, 0xE6, 0xED, 0x03, 0xCE, 0x44, 0x37, 0xA2, 0xC4,
        0xD4, 0xA0, 0x6B, 0xFA, 0x58, 0x23, 0xF1, 0xB8, 0x87, 0x9B, 0xAE, 0x9D,
        0xFF, 0x68, 0xE3, 0x7A, 0xC4, 0x18, 0xEF, 0x32, 0x2E, 0xC2, 0xAB, 0x35,
        0xB3, 0x31, 0x52, 0x03, 0x5D, 0xC3, 0x4C, 0xF3
};

static const unsigned char RSA_DQ[] = {
        0x47, 0x1B, 0xD4, 0xC1, 0xC6, 0x47, 0x04, 0x50, 0x5F, 0xBD, 0x01, 0xE3,
        0x0E, 0x76, 0x87, 0xE7, 0xF0, 0xC7, 0x68, 0x3A, 0xED, 0x20, 0x72, 0xE3,
        0x87, 0x43, 0xAD, 0x85, 0x36, 0x4C, 0x61, 0xC9, 0xC7, 0xD9, 0xCA, 0x0A,
        0x25, 0xE7, 0x92, 0x5F, 0x2C, 0x1D, 0x67, 0x08, 0x1E, 0xF9, 0x9C, 0xF1,
        0x68, 0xBC, 0xCB, 0xF3, 0x31, 0x82, 0x78, 0x62, 0x33, 0x5C, 0xC1, 0xE1,
        0x77, 0xE4, 0x64, 0x08, 0x22, 0x77, 0xA2, 0xA8, 0xC3, 0xCC, 0x8B, 0x05,
        0x36, 0x9F, 0x22, 0x37, 0x52, 0x11, 0x34, 0x60, 0xB9, 0x42, 0x1F, 0x6D,
        0x15, 0x84, 0xE6, 0x16, 0xCE, 0x59, 0xFE, 0x2B, 0x3F, 0x2C, 0xE0, 0x6F,
        0xE5, 0xD1, 0xEF, 0x12, 0x9D, 0x84, 0xAE, 0xCA, 0xEE, 0x09, 0x6E, 0xEB,
        0x61, 0x69, 0x15, 0x9F, 0x8E, 0x28, 0xB1, 0x3F, 0x71, 0xE4, 0xF8, 0xFF,
        0xFC, 0x32, 0x86, 0x39, 0x29, 0x82, 0x86, 0x77
};

static const unsigned char RSA_IQ[] = {
        0x6B, 0x9C, 0x12, 0x57, 0x6C, 0x3D, 0x20, 0xA7, 0x43, 0x86, 0x4B, 0xD7,
        0xE1, 0xFB, 0x1F, 0x1A, 0xAF, 0x7A, 0xF6, 0xC3, 0x9C, 0x31, 0xAD, 0x58,
        0xEB, 0x85, 0xF8, 0xA4, 0x60, 0xED, 0x54, 0x95, 0x46, 0xBE, 0x97, 0x68,
        0xCE, 0xC6, 0x37, 0xA1, 0xD8, 0xF0, 0x7E, 0x88, 0x12, 0xD2, 0xB7, 0x41,
        0x6A, 0x39, 0xC6, 0x67, 0x4C, 0xFC, 0xB6, 0x92, 0x4B, 0xDD, 0x1F, 0x18,
        0x00, 0x41, 0x0D, 0x17, 0x40, 0x59, 0xF3, 0xB2, 0x7E, 0x11, 0x65, 0x16,
        0x03, 0xD5, 0xFB, 0xEF, 0x49, 0x0B, 0xD6, 0xEF, 0x56, 0xA2, 0x20, 0x7F,
        0x9C, 0x2B, 0x21, 0xEE, 0xB2, 0xFF, 0xD6, 0x0F, 0x37, 0x1C, 0x0E, 0x57,
        0x34, 0xB7, 0x7D, 0xAA, 0x04, 0x04, 0x46, 0x8A, 0x94, 0xE2, 0x51, 0x51,
        0xCC, 0xE0, 0xF8, 0x1D, 0x5C, 0xED, 0x4A, 0xF0, 0xBB, 0x37, 0x5A, 0x08,
        0x99, 0x6C, 0x28, 0x7E, 0x14, 0xB3, 0x18, 0xD0
};

static const br_rsa_private_key RSA = {
        2048,
        (unsigned char *)RSA_P, sizeof RSA_P,
        (unsigned char *)RSA_Q, sizeof RSA_Q,
        (unsigned char *)RSA_DP, sizeof RSA_DP,
        (unsigned char *)RSA_DQ, sizeof RSA_DQ,
        (unsigned char *)RSA_IQ, sizeof RSA_IQ
};


static const unsigned char CERT0[] = {
        0x30, 0x82, 0x04, 0x10, 0x30, 0x82, 0x02, 0xF8, 0xA0, 0x03, 0x02, 0x01,
        0x02, 0x02, 0x14, 0x50, 0x1F, 0x7A, 0x99, 0xE4, 0x57, 0x79, 0x9B, 0x7E,
        0xFD, 0xB5, 0x27, 0x01, 0x0F, 0x54, 0x40, 0xF3, 0xF8, 0x1E, 0xE7, 0x30,
        0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01, 0x0B,
        0x05, 0x00, 0x30, 0x81, 0x88, 0x31, 0x0B, 0x30, 0x09, 0x06, 0x03, 0x55,
        0x04, 0x06, 0x13, 0x02, 0x43, 0x4C, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03,
        0x55, 0x04, 0x08, 0x0C, 0x08, 0x53, 0x61, 0x6E, 0x74, 0x69, 0x61, 0x67,
        0x6F, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04, 0x07, 0x0C, 0x08,
        0x53, 0x61, 0x6E, 0x74, 0x69, 0x61, 0x67, 0x6F, 0x31, 0x10, 0x30, 0x0E,
        0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x4E, 0x49, 0x43, 0x4C, 0x41,
        0x42, 0x53, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55, 0x04, 0x0B, 0x0C,
        0x03, 0x49, 0x4F, 0x54, 0x31, 0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04,
        0x03, 0x0C, 0x07, 0x47, 0x61, 0x62, 0x72, 0x69, 0x65, 0x6C, 0x31, 0x21,
        0x30, 0x1F, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x09,
        0x01, 0x16, 0x12, 0x67, 0x61, 0x62, 0x72, 0x69, 0x65, 0x6C, 0x40, 0x6E,
        0x69, 0x63, 0x6C, 0x61, 0x62, 0x73, 0x2E, 0x63, 0x6C, 0x30, 0x1E, 0x17,
        0x0D, 0x32, 0x30, 0x30, 0x31, 0x32, 0x31, 0x31, 0x39, 0x32, 0x36, 0x33,
        0x35, 0x5A, 0x17, 0x0D, 0x32, 0x31, 0x30, 0x31, 0x32, 0x30, 0x31, 0x39,
        0x32, 0x36, 0x33, 0x35, 0x5A, 0x30, 0x81, 0x88, 0x31, 0x0B, 0x30, 0x09,
        0x06, 0x03, 0x55, 0x04, 0x06, 0x13, 0x02, 0x43, 0x4C, 0x31, 0x11, 0x30,
        0x0F, 0x06, 0x03, 0x55, 0x04, 0x08, 0x0C, 0x08, 0x53, 0x61, 0x6E, 0x74,
        0x69, 0x61, 0x67, 0x6F, 0x31, 0x11, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x04,
        0x07, 0x0C, 0x08, 0x53, 0x61, 0x6E, 0x74, 0x69, 0x61, 0x67, 0x6F, 0x31,
        0x10, 0x30, 0x0E, 0x06, 0x03, 0x55, 0x04, 0x0A, 0x0C, 0x07, 0x4E, 0x49,
        0x43, 0x4C, 0x41, 0x42, 0x53, 0x31, 0x0C, 0x30, 0x0A, 0x06, 0x03, 0x55,
        0x04, 0x0B, 0x0C, 0x03, 0x49, 0x4F, 0x54, 0x31, 0x10, 0x30, 0x0E, 0x06,
        0x03, 0x55, 0x04, 0x03, 0x0C, 0x07, 0x47, 0x61, 0x62, 0x72, 0x69, 0x65,
        0x6C, 0x31, 0x21, 0x30, 0x1F, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7,
        0x0D, 0x01, 0x09, 0x01, 0x16, 0x12, 0x67, 0x61, 0x62, 0x72, 0x69, 0x65,
        0x6C, 0x40, 0x6E, 0x69, 0x63, 0x6C, 0x61, 0x62, 0x73, 0x2E, 0x63, 0x6C,
        0x30, 0x82, 0x01, 0x22, 0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86,
        0xF7, 0x0D, 0x01, 0x01, 0x01, 0x05, 0x00, 0x03, 0x82, 0x01, 0x0F, 0x00,
        0x30, 0x82, 0x01, 0x0A, 0x02, 0x82, 0x01, 0x01, 0x00, 0xCF, 0xAC, 0x7E,
        0x75, 0x6C, 0x93, 0x5A, 0xF4, 0x19, 0x7C, 0xE4, 0x9A, 0x4A, 0x8A, 0x17,
        0x4C, 0x88, 0x5D, 0x27, 0xDE, 0x13, 0x3F, 0xD3, 0xC7, 0xFD, 0x25, 0x68,
        0x51, 0x86, 0x9F, 0x55, 0x23, 0xCC, 0xF5, 0x2F, 0xFC, 0xA5, 0x0F, 0x23,
        0xE0, 0x4B, 0x7D, 0x76, 0x37, 0x27, 0x53, 0xC8, 0x26, 0xC3, 0xAE, 0x0C,
        0x22, 0x2C, 0x25, 0x8F, 0x9F, 0xBB, 0x79, 0x96, 0x7B, 0xFA, 0x9B, 0x78,
        0x4B, 0xBF, 0xCA, 0x6D, 0xC9, 0xE3, 0x8E, 0x2F, 0x1F, 0x8B, 0xA6, 0xF6,
        0x16, 0x75, 0xCE, 0xA8, 0xAF, 0x77, 0x3D, 0x0F, 0xF0, 0xB6, 0x79, 0x4A,
        0x02, 0xC5, 0xE2, 0x59, 0x50, 0x42, 0x47, 0x3E, 0x83, 0x83, 0x1C, 0x48,
        0xC0, 0xFF, 0xF9, 0xC1, 0x53, 0x3F, 0x04, 0x9E, 0x89, 0x67, 0x17, 0xF3,
        0x63, 0x6D, 0xC7, 0x9F, 0x52, 0x5F, 0x8E, 0x28, 0x25, 0xF5, 0x77, 0xFA,
        0xD4, 0x58, 0x0C, 0xE6, 0xD7, 0xA5, 0x29, 0x5B, 0xA9, 0x09, 0xBB, 0x73,
        0xFC, 0x23, 0x71, 0x27, 0xE4, 0xE8, 0x7C, 0x3F, 0xBA, 0x4D, 0x20, 0xE1,
        0xC1, 0x7F, 0xDA, 0xD7, 0x13, 0x58, 0x98, 0x79, 0xDD, 0x4A, 0xA9, 0x43,
        0x43, 0x5A, 0xA8, 0xA6, 0xFA, 0x2B, 0x0B, 0x0B, 0x74, 0x1A, 0xE9, 0xDB,
        0x40, 0x42, 0x67, 0xE3, 0xB7, 0xDA, 0xDA, 0xB9, 0x7D, 0x0C, 0x9A, 0x26,
        0x11, 0xD4, 0x73, 0xDF, 0xC3, 0x69, 0x5D, 0xB4, 0x03, 0xFA, 0x13, 0x07,
        0xE0, 0x71, 0xDC, 0x65, 0x33, 0x55, 0xAB, 0xE9, 0xDA, 0xC2, 0x5F, 0x0A,
        0xC8, 0x75, 0x9B, 0xB9, 0x57, 0xBA, 0x15, 0xDC, 0x52, 0xB5, 0xF8, 0xF7,
        0x51, 0xE0, 0xEC, 0xCA, 0x45, 0x8D, 0xF4, 0x02, 0x8D, 0x4C, 0x54, 0xD3,
        0xD5, 0x38, 0x25, 0x1F, 0xC6, 0x4C, 0xA6, 0x6C, 0x03, 0x83, 0x95, 0xAE,
        0x02, 0xDF, 0x61, 0x03, 0x12, 0x14, 0xEB, 0xCA, 0x82, 0xE9, 0xEB, 0xE2,
        0x55, 0x02, 0x03, 0x01, 0x00, 0x01, 0xA3, 0x70, 0x30, 0x6E, 0x30, 0x1D,
        0x06, 0x03, 0x55, 0x1D, 0x0E, 0x04, 0x16, 0x04, 0x14, 0x65, 0xFF, 0x57,
        0x9F, 0x7B, 0xCF, 0x31, 0x3F, 0x09, 0xAD, 0xC5, 0x20, 0x2A, 0xC5, 0x05,
        0x08, 0xC0, 0x4F, 0x4F, 0x50, 0x30, 0x1F, 0x06, 0x03, 0x55, 0x1D, 0x23,
        0x04, 0x18, 0x30, 0x16, 0x80, 0x14, 0x65, 0xFF, 0x57, 0x9F, 0x7B, 0xCF,
        0x31, 0x3F, 0x09, 0xAD, 0xC5, 0x20, 0x2A, 0xC5, 0x05, 0x08, 0xC0, 0x4F,
        0x4F, 0x50, 0x30, 0x0F, 0x06, 0x03, 0x55, 0x1D, 0x13, 0x01, 0x01, 0xFF,
        0x04, 0x05, 0x30, 0x03, 0x01, 0x01, 0xFF, 0x30, 0x1B, 0x06, 0x03, 0x55,
        0x1D, 0x11, 0x04, 0x14, 0x30, 0x12, 0x87, 0x10, 0xFD, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x03, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08,
        0x30, 0x0D, 0x06, 0x09, 0x2A, 0x86, 0x48, 0x86, 0xF7, 0x0D, 0x01, 0x01,
        0x0B, 0x05, 0x00, 0x03, 0x82, 0x01, 0x01, 0x00, 0x23, 0x4C, 0x9B, 0xAB,
        0x72, 0x3C, 0x4C, 0x34, 0x39, 0xEF, 0xE0, 0x27, 0x07, 0x42, 0x0D, 0x9D,
        0x78, 0xC1, 0xCF, 0x88, 0x33, 0x99, 0x2C, 0x27, 0xED, 0xD5, 0xC0, 0x1D,
        0x08, 0xA0, 0x5E, 0xFB, 0x74, 0x8C, 0x84, 0x5B, 0x25, 0xD7, 0x6D, 0xCE,
        0xE4, 0x89, 0x86, 0x09, 0x63, 0x89, 0x84, 0x0C, 0x43, 0x83, 0x96, 0x4B,
        0xFD, 0x21, 0xBA, 0xF4, 0x3E, 0xCE, 0x49, 0x17, 0xCA, 0x5D, 0xBC, 0x90,
        0x44, 0xFF, 0xF7, 0xA0, 0x1D, 0xBF, 0xDF, 0x4C, 0xAF, 0xD8, 0x60, 0x6D,
        0x38, 0xDC, 0x5A, 0x86, 0x2E, 0xC2, 0x8B, 0x7C, 0xAA, 0x9A, 0xEF, 0x40,
        0xB2, 0xAC, 0x00, 0x10, 0x68, 0x23, 0xCD, 0xCC, 0xA5, 0xF6, 0x5F, 0x1A,
        0x84, 0x95, 0x02, 0x89, 0x67, 0x0E, 0xBF, 0x43, 0x69, 0xA5, 0xC8, 0x81,
        0x35, 0x73, 0x3C, 0x77, 0xB3, 0xDB, 0x5B, 0x17, 0x0B, 0x7E, 0x43, 0x69,
        0x86, 0x58, 0x4E, 0x10, 0xE7, 0x75, 0xED, 0x06, 0x8E, 0x7A, 0x9A, 0xA5,
        0xE0, 0x59, 0xE5, 0x96, 0x4D, 0x2C, 0x6C, 0xF9, 0xEC, 0xCC, 0x76, 0x41,
        0xE0, 0x01, 0xAE, 0x5D, 0xBE, 0xF1, 0xA4, 0x34, 0xD3, 0x35, 0xEE, 0x90,
        0x9E, 0xE9, 0x13, 0x68, 0x5F, 0x97, 0x26, 0xF4, 0xE9, 0xB3, 0xF8, 0x82,
        0x92, 0x71, 0x4F, 0x53, 0x0E, 0xA2, 0x34, 0xA7, 0x9C, 0x5F, 0xF2, 0xF6,
        0x20, 0x84, 0x6F, 0x45, 0xFC, 0xD9, 0x40, 0x35, 0x03, 0xE4, 0x89, 0xEC,
        0xC3, 0xF4, 0x13, 0x96, 0x1B, 0x8A, 0xD4, 0xA4, 0x71, 0x19, 0x08, 0xED,
        0x8A, 0x8F, 0x14, 0x9F, 0xE4, 0xA3, 0x7D, 0x48, 0x96, 0xE3, 0x05, 0xE9,
        0xDD, 0xF1, 0x1F, 0x44, 0x1B, 0x29, 0xF2, 0x08, 0x60, 0xAC, 0x80, 0x39,
        0x8D, 0xDD, 0xF1, 0x49, 0xE0, 0x8A, 0xE0, 0x7D, 0x01, 0xF7, 0x35, 0x79,
        0x45, 0xBB, 0xF5, 0x05, 0x85, 0xAE, 0x2D, 0xC5, 0x10, 0x0C, 0x66, 0x76
};

static const br_x509_certificate CHAIN[] = {
        { (unsigned char *)CERT0, sizeof CERT0 }
};

#define CHAIN_LEN   1
char *supported_protocols = "h2";

void init_tls_server(http2_ctx_t *ctx)
{
    br_ssl_server_context *sc = &ctx->sc;

    ctx->handshake = 0; //handshake not done
    br_ssl_server_zero(sc);

    //br_ssl_server_init_full_rsa(sc, CHAIN, CHAIN_LEN, &RSA);
    br_ssl_server_init_mine2g(sc, CHAIN, CHAIN_LEN, &RSA);
    br_ssl_engine_set_versions(&sc->eng, BR_TLS12, BR_TLS12); //ONLY TLS 1.2

    uint32_t flags = 0;
    flags |= BR_OPT_ENFORCE_SERVER_PREFERENCES;
    flags |= BR_OPT_NO_RENEGOTIATION;
    flags |= BR_OPT_TOLERATE_NO_CLIENT_AUTH;
    flags |= BR_OPT_FAIL_ON_ALPN_MISMATCH;
    br_ssl_engine_set_all_flags(&sc->eng, flags);
    memset(ctx->iobuf_in,0, BR_SSL_BUFSIZE_INPUT);
    memset(ctx->iobuf_out,0, BR_SSL_BUFSIZE_INPUT);
    br_ssl_engine_set_buffers_bidi(&sc->eng,
                                   ctx->iobuf_in, sizeof ctx->iobuf_in,
                                   ctx->iobuf_out, sizeof ctx->iobuf_out);
    //char *supported_protocols = "h2";
    br_ssl_engine_set_protocol_names(&sc->eng, (const char **)&supported_protocols, 1);
}

/*
 * Inspect the provided data in case it is a "command" to trigger a
 * special behaviour. If the command is recognised, then it is executed
 * and this function returns 1. Otherwise, this function returns 0.
 */

static int
run_command(br_ssl_engine_context *cc, unsigned char *buf, size_t len)
{
    /*
     * A single static slot for saving session parameters.
     */
    static br_ssl_session_parameters slot;
    static int slot_used = 0;

    size_t u;

    if (len < 2 || len > 3) {
        return 0;
    }
    if (len == 3 && (buf[1] != '\r' || buf[2] != '\n')) {
        return 0;
    }
    if (len == 2 && buf[1] != '\n') {
        return 0;
    }
    switch (buf[0]) {
        case 'Q':
            DEBUG("closing...\n");
            br_ssl_engine_close(cc);
            return 1;
        case 'R':
            if (br_ssl_engine_renegotiate(cc)) {
                DEBUG("renegotiating...\n");
            }
            else {
                DEBUG("not renegotiating.\n");
            }
            return 1;
        case 'F':
            /*
             * Session forget is nominally client-only. But the
             * session parameters are in the engine structure, which
             * is the first field of the client context, so the cast
             * still works properly. On the server, this forgetting
             * has no effect.
             */
            DEBUG("forgetting session...\n");
            br_ssl_client_forget_session((br_ssl_client_context *)cc);
            return 1;
        case 'S':
            DEBUG("saving session parameters...\n");
            br_ssl_engine_get_session_parameters(cc, &slot);
            DEBUG("  id = ");
            for (u = 0; u < slot.session_id_len; u++) {
                DEBUG("%02X", slot.session_id[u]);
            }
            DEBUG( "\n");
            slot_used = 1;
            return 1;
        case 'P':
            if (slot_used) {
                DEBUG("restoring session parameters...\n");
                DEBUG("  id = ");
                for (u = 0; u < slot.session_id_len; u++) {
                    DEBUG("%02X", slot.session_id[u]);
                }
                DEBUG("\n");
                br_ssl_engine_set_session_parameters(cc, &slot);
                return 1;
            }
            return 0;
        default:
            return 0;
    }
}

int engine_error(br_ssl_server_context *cc)
{
    int err = br_ssl_engine_last_error(&cc->eng);

    if (err == BR_ERR_OK) {
        return 0;
    }
    else {
        DEBUG("ERROR: SSL error %d", err);
        if (err >= BR_ERR_SEND_FATAL_ALERT) {
            err -= BR_ERR_SEND_FATAL_ALERT;
            DEBUG(" (sent alert %d)\n", err);
        }
        else if (err >= BR_ERR_RECV_FATAL_ALERT) {
            err -= BR_ERR_RECV_FATAL_ALERT;
            DEBUG(" (received alert %d)\n", err);
        }
        else {
            const char *ename;
            ename = "unknown";
            DEBUG(" (%s)\n", ename);
        }
        return err;
    }
}

void write_ssl_handshake(event_sock_t *client, int status);
void on_client_close(event_sock_t *handle);
int read_preface(event_sock_t *client, int size, uint8_t *buf);
void write_ssl(event_sock_t *client, int status);
void on_settings_sent(event_sock_t *client, int status);
int read_header(event_sock_t *client, int size, uint8_t *buf);
int read_settings_payload(event_sock_t *client, int size, uint8_t *buf);
int read_ssl_data(event_sock_t *client, int size, uint8_t *buf);

int send_app_ssl_data(event_sock_t *client, int size, uint8_t *buf){
   // DEBUG("I'm here :) (SENDAPP)");

    http2_ctx_t *ctx = (http2_ctx_t *)client->data;
    br_ssl_server_context *sc = &ctx->sc;
    br_ssl_engine_context *cc = &sc->eng;
    int sendapp;
    unsigned st;
    /*
     * Get current engine state.
     */
    st = br_ssl_engine_current_state(cc);
    sendapp = ((st & BR_SSL_SENDAPP) != 0);

   // DEBUG("sendapp is %d",sendapp);
    //If engine is closed
    if (st == BR_SSL_CLOSED) {
        //DEBUG("ERROR IN CALLBACK WHILE");
        event_close(client, on_client_close);
        return engine_error(sc);
    }
    uint32_t count = 0;
    while (sendapp && count < size) {
        //DEBUG("sendapp");

        unsigned char *ssl_buf;
        size_t len;

        ssl_buf = br_ssl_engine_sendapp_buf(cc, &len);
        len = len > size ? size : len;
        //DEBUG("LEN IS %d", len);
        //DEBUG("SIZE IS %d",size);
        memcpy(ssl_buf, buf, len);
        if (!run_command(cc, ssl_buf, len)) {
            br_ssl_engine_sendapp_ack(cc, len);
            count += len;
        }
        br_ssl_engine_flush(cc, 0);
        return len;
    }
    event_read(client, read_ssl_data);
    return count;
}

int read_ssl_data(event_sock_t *client, int size, uint8_t *buf){
   // DEBUG("I'm here :)");
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;
    br_ssl_server_context *sc = &ctx->sc;
    br_ssl_engine_context *cc = &sc->eng;
    int sendrec, recvrec, sendapp, recvapp;
    unsigned st;
    /*
     * Get current engine state.
     */
    st = br_ssl_engine_current_state(cc);
    sendrec = ((st & BR_SSL_SENDREC) != 0);
    recvrec = ((st & BR_SSL_RECVREC) != 0);
    sendapp = ((st & BR_SSL_SENDAPP) != 0);
    recvapp = ((st & BR_SSL_RECVAPP) != 0);

    //DEBUG("sendrec is %d",sendrec);
   // DEBUG("recvrec is %d",recvrec);
   // DEBUG("sendapp is %d",sendapp);
    //DEBUG("recvapp is %d",recvapp);
    //If engine is closed
    if (st == BR_SSL_CLOSED) {
       // DEBUG("ERROR IN CALLBACK WHILE");
        event_close(client, on_client_close);
        return engine_error(sc);
    }
    if (size >= 0) {
        if (recvapp) {
      //      DEBUG("RECVAPP");
            unsigned char *buf;
            size_t len;

            buf = br_ssl_engine_recvapp_buf(cc, &len);
            //HTTP2_IDLE,
            //        HTTP2_WAITING_PREFACE,
            //        HTTP2_WAITING_SETTINGS
           // DEBUG("HTTP2 state %d", ctx->state);
            if (ctx->state == HTTP2_WAITING_PREFACE) {
                len = read_preface(client, len, (uint8_t *) buf);
            } else if(ctx->state ==HTTP2_WAITING_SETTINGS){
                len = read_header(client, len, (uint8_t *) buf);
            } else if(ctx->state == HTTP2_WAITING_SETTINGS_PAYLOAD){
                len = read_settings_payload(client, len, (uint8_t *) buf);
            }
            else {
                DEBUG("ELSE AHHH ");
            }
            //event_write(ctx->sock, len, buf, on_ssl_sent);
            DEBUG("HTTP2 read %d bytes", len);
            br_ssl_engine_recvapp_ack(cc, len);
            event_read(client, read_ssl_data);

            return 0;
        }
        if (sendrec) {
            //DEBUG("sendrecREADSSL1");
            size_t len;
            buf = br_ssl_engine_sendrec_buf(cc, &len);
            //DEBUG("Len is %d", len);
            len = len < HTTP2_MAX_FRAME_SIZE ? len : HTTP2_MAX_FRAME_SIZE;
            //DEBUG("WLen is %d",len);
            //DEBUG("HTTP2 state %d", ctx->state);
            uint32_t wlen;

            if (ctx->state == HTTP2_WAITING_PREFACE) {
                //DEBUG("WAITING PREFACE");
                wlen = event_write(ctx->sock, len, buf, on_settings_sent);
            } else {
                //DEBUG("ELSE AHHH ");
                wlen = event_write(ctx->sock, len, buf, write_ssl);
            }
            br_ssl_engine_sendrec_ack(cc, wlen);
            return 0;
        }
        if (recvrec) {
            //DEBUG("recvrec");

            unsigned char *ssl_buf;
            size_t len;
            ssl_buf = br_ssl_engine_recvrec_buf(cc, &len);
            len = size < len ? size : len;
            memcpy(ssl_buf, buf, len);
           // DEBUG("HTTP2 state %d", ctx->state);

            br_ssl_engine_recvrec_ack(cc, len);

            st = br_ssl_engine_current_state(cc);
            sendrec = ((st & BR_SSL_SENDREC) != 0);
            recvrec = ((st & BR_SSL_RECVREC) != 0);
            sendapp = ((st & BR_SSL_SENDAPP) != 0);
            recvapp = ((st & BR_SSL_RECVAPP) != 0);

           // DEBUG("sendrec is %d",sendrec);
          //  DEBUG("recvrec is %d",recvrec);
          //  DEBUG("sendapp is %d",sendapp);
          //  DEBUG("recvapp is %d",recvapp);
            //If engine is closed
            if (st == BR_SSL_CLOSED) {
             //   DEBUG("ERROR IN CALLBACK WHILE");
                event_close(client, on_client_close);
                return engine_error(sc);
            }
            if(recvapp && ctx->state != HTTP2_WAITING_PREFACE){
             //   DEBUG("RECVAPP2");
                buf = br_ssl_engine_recvapp_buf(cc, &len);
                //HTTP2_IDLE,
                //        HTTP2_WAITING_PREFACE,
                //        HTTP2_WAITING_SETTINGS
             //   DEBUG("HTTP2 state %d", ctx->state);
                if(ctx->state ==HTTP2_WAITING_SETTINGS){
                    len = read_header(client, len, (uint8_t *) buf);
                } else if(ctx->state == HTTP2_WAITING_SETTINGS_PAYLOAD){
                    len = read_settings_payload(client, len, (uint8_t *) buf);
                }
                else {
                    DEBUG("ELSE AHHH ");
                }
                //event_write(ctx->sock, len, buf, on_ssl_sent);
                DEBUG("HTTP2 read %d bytes", len);
                br_ssl_engine_recvapp_ack(cc, len);
            }
            event_read(client, read_ssl_data);
            return len;
        }
        if (sendapp) {
            //DEBUG("sendappLOOP");
/*
            unsigned char *ssl_buf;
            size_t len;

            ssl_buf = br_ssl_engine_sendapp_buf(cc, &len);
            len = len > size ? size : len;
            memcpy(ssl_buf, buf, len);
            if (!run_command(cc, ssl_buf, len)) {
                br_ssl_engine_sendapp_ack(cc, len);
            }
            br_ssl_engine_flush(cc, 0);
            return len;*/
            return 0;
        }
    }
    else if (size < 0) {
        INFO("Remote client closed");
        event_close(client, on_client_close);
    }
    return 0;
}

int read_ssl_handshake(event_sock_t *client, int size, uint8_t *buf)
{
    // read context from client data
    assert(client->data != NULL);
    //DEBUG("In callback");
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;
    br_ssl_server_context *sc = &ctx->sc;
    br_ssl_engine_context *cc = &sc->eng;
    unsigned st;
    int sendrec, recvrec, sendapp;
    /*
     * Get current engine state.
     */
    st = br_ssl_engine_current_state(cc);
    //If engine is closed
    if (st == BR_SSL_CLOSED) {
        DEBUG("ERROR IN CALLBACK WHILE");
        event_close(client, on_client_close);

        return engine_error(sc);
    }
    //sendrec = ((st & BR_SSL_SENDREC) != 0);
    recvrec = ((st & BR_SSL_RECVREC) != 0);
    sendapp = ((st & BR_SSL_SENDAPP) != 0);
    //recvapp = ((st & BR_SSL_RECVAPP) != 0);
    uint8_t *handshake = &ctx->handshake;
    //DEBUG("Handshake is %d",*handshake);
    if (sendapp) {
        const char *pname;

        INFO("Handshake completed");
        INFO("   version:               ");
        switch (br_ssl_engine_get_version(cc)) {
            case BR_SSL30:
                INFO("   SSL 3.0");
                break;
            case BR_TLS10:
                INFO("   TLS 1.0");
                break;
            case BR_TLS11:
                INFO("   TLS 1.1");
                break;
            case BR_TLS12:
                INFO("   TLS 1.2");
                break;
            default:
                INFO("   unknown (0x%04X)",
                      (unsigned)cc->session.version);
                break;
        }
        INFO("   secure renegotiation:  %s",
              cc->reneg == 1 ? "no" : "yes");
        pname = br_ssl_engine_get_selected_protocol(cc);
        if (pname != NULL) {
            INFO("   protocol name (ALPN):  %s",
                  pname);
        }
        event_read(client, read_ssl_data);
        return 0;
    }
    if (size >= 0 || *handshake < 2) {
        if (recvrec) {
            //DEBUG("recvrec");

            unsigned char *ssl_buf;
            uint32_t count = 0;
            while(recvrec && count < size) {
                size_t len;
                ssl_buf = br_ssl_engine_recvrec_buf(cc, &len);
                if (size < 0) {
                    return 0;
                }
                len = size < len ? size : len;
                memcpy(ssl_buf, buf+count, len);
                count += len;
                br_ssl_engine_recvrec_ack(cc, len);
                st = br_ssl_engine_current_state(cc);
                recvrec = ((st & BR_SSL_RECVREC) != 0);

            }
            // prepare response
            if(*handshake<=1){
                size_t len;
                st = br_ssl_engine_current_state(cc);
                //If engine is closed
                if (st == BR_SSL_CLOSED) {
                    return engine_error(sc);
                }
                sendrec = ((st & BR_SSL_SENDREC) != 0);
                if(sendrec) {
                    //DEBUG("sendrec2");
                    buf = br_ssl_engine_sendrec_buf(cc, &len);
                    len = len < HTTP2_MAX_FRAME_SIZE ? len : HTTP2_MAX_FRAME_SIZE;
                    uint32_t wlen = event_write(ctx->sock, len, buf, write_ssl_handshake);
                    //DEBUG("L is %d", wlen);
                    br_ssl_engine_sendrec_ack(cc, wlen);
                    st = br_ssl_engine_current_state(cc);
                    sendrec = ((st & BR_SSL_SENDREC) != 0);
                    if(!sendrec){
                        *handshake+=1;
                    }
                }
            }
            event_read(client, read_ssl_handshake);
            return count;
        }
    }
    else if (size < 0) {
        INFO("Remote client closed");
        event_close(client, on_client_close);
    }
    return 0;
}
#endif // ENABLE_SSL

// define the process here
#ifdef CONTIKI
PROCESS(tiny_server_process, "Tiny server process");
AUTOSTART_PROCESSES(&tiny_server_process);
#endif

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

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    exit(0);
#elif defined(CONTIKI)
    process_exit(&tiny_server_process);
#endif
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
#if TLS_ENABLE
void write_ssl_handshake(event_sock_t *client, int status)
{
    //DEBUG("On ssl sent");
    if (status < 0) {
        ERROR("Could not send ssl ack");
        event_close(client, on_client_close);
        return;
    }
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;
    br_ssl_server_context *sc = &ctx->sc;
    br_ssl_engine_context *cc = &sc->eng;
    unsigned st;
    int sendrec;
    /*
     * Get current engine state.
     */
    st = br_ssl_engine_current_state(cc);
    //If engine is closed
    if (st == BR_SSL_CLOSED) {
       // DEBUG("ERROR IN CALLBACK WHILE");
        engine_error(sc);
        return;
    }
    sendrec = ((st & BR_SSL_SENDREC) != 0);
    //recvapp = ((st & BR_SSL_RECVAPP) != 0);
    uint8_t *handshake = &ctx->handshake;
   // DEBUG("Handshake is %d",*handshake);
    if (sendrec) {
        //DEBUG("sendrecW");

        unsigned char *buf;
        size_t len;

        buf = br_ssl_engine_sendrec_buf(cc, &len);
        //DEBUG("Len is %d", len);
        len = len < HTTP2_MAX_FRAME_SIZE ? len : HTTP2_MAX_FRAME_SIZE;
        //DEBUG("WLen is %d",len);
        uint32_t wlen = event_write(ctx->sock, len, buf, write_ssl_handshake);

        br_ssl_engine_sendrec_ack(cc, wlen);
        st = br_ssl_engine_current_state(cc);
        sendrec = ((st & BR_SSL_SENDREC) != 0);
        if(!sendrec){
            *handshake+=1;
        }
    }
}

void write_ssl(event_sock_t *client, int status)
{
    //DEBUG("On write_ssl");
    if (status < 0) {
        ERROR("Could not send ssl record");
        event_close(client, on_client_close);
        return;
    }
    http2_ctx_t *ctx = (http2_ctx_t *)client->data;
    br_ssl_server_context *sc = &ctx->sc;
    br_ssl_engine_context *cc = &sc->eng;
    unsigned st;
    int sendrec;
    /*
     * Get current engine state.
     */
    st = br_ssl_engine_current_state(cc);
    //If engine is closed
    if (st == BR_SSL_CLOSED) {
        //DEBUG("ERROR IN CALLBACK WHILE");
        engine_error(sc);
        return;
    }
    sendrec = ((st & BR_SSL_SENDREC) != 0);
    //recvapp = ((st & BR_SSL_RECVAPP) != 0);
    if (sendrec) {
       // DEBUG("sendrecWRITESSL");
        unsigned char* buf;
        size_t len;
        buf = br_ssl_engine_sendrec_buf(cc, &len);
//DEBUG("Len is %d", len);
        len = len < HTTP2_MAX_FRAME_SIZE ? len : HTTP2_MAX_FRAME_SIZE;
//DEBUG("WLen is %d",len);
        uint32_t wlen = event_write(ctx->sock, len, buf, write_ssl);
        br_ssl_engine_sendrec_ack(cc, wlen);
    }
    return;
}
#endif//ENABLE SSL

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

void send_settings_frame(http2_ctx_t *ctx, struct frame_settings_field *fields, int len, int ack, event_write_cb on_send)
{
    assert(ctx->sock != NULL);
    assert(fields == NULL || ack == false);

    if (ack) {
        DEBUG("SENDING SETTINGS ACK");
        raw_frame_header_t hd;
        create_frame_header(&hd, 0, ack, SETTINGS_FRAME, 0);
#if TLS_ENABLE
        send_app_ssl_data(ctx->sock, HTTP2_HEADER_SIZE, hd.data);
#else
        event_write(ctx->sock, HTTP2_HEADER_SIZE, hd.data, on_send);
#endif
    }
    else {
        DEBUG("SENDING SETTINGS");
        int size = len * sizeof(struct frame_settings_field);

        raw_frame_header_t hd;
        create_frame_header(&hd, size, 0, SETTINGS_FRAME, 0);

        uint8_t buf[HTTP2_HEADER_SIZE + size];
        memcpy(buf, hd.data, HTTP2_HEADER_SIZE);
        memcpy(buf + HTTP2_HEADER_SIZE, &fields, size - HTTP2_HEADER_SIZE);

#if TLS_ENABLE
        send_app_ssl_data(ctx->sock, size, buf);
#else
        event_write(ctx->sock, size, buf, on_send);
#endif
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


int read_settings_payload(event_sock_t *client, int size, uint8_t *buf)
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

        DEBUG("read SETTINGS payload");
        // send settings ack
        send_settings_frame(ctx, NULL, 0, true, on_settings_ack_sent);

        return ctx->header.length;
    }
    else if (size <= 0) {
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
                //event_read(client, read_settings_payload);
                //new code
#if TLS_ENABLE
                ctx->state=HTTP2_WAITING_SETTINGS_PAYLOAD;
                event_read(client, read_ssl_data);
#else
                event_read(client, read_settings_payload);
#endif
            }
            break;
        default:
            ERROR("Unexpected state");
            return -1;
    }
    return 0;
}

int read_header(event_sock_t *client, int size, uint8_t *buf)
{
    if (size >= HTTP2_HEADER_SIZE) {
        // get client context from socket data
        assert(client->data != NULL);
        http2_ctx_t *ctx = (http2_ctx_t *)client->data;

        // Copy header data to current frame
        memcpy(ctx->frame, buf, HTTP2_HEADER_SIZE);

        frame_header_t *header = &ctx->header;
        parse_frame_header(ctx->frame, header);

#ifdef __arm__
        DEBUG("received header = {length: %ld, type: %d, flags: %d, stream_id: %ld}", \
              header->length, header->type, header->flags, header->stream_id);
#else
        DEBUG("received header = {length: %d, type: %d, flags: %d, stream_id: %d}", \
              header->length, header->type, header->flags, header->stream_id);
#endif

        if ((size - 9)  < header->length && process_header(client, ctx, header) < 0) {
            event_close(client, on_client_close);
        } else {
            return 9 + read_settings_payload(client, size - 9, buf + 9);
        }

        return HTTP2_HEADER_SIZE;
    }
    else if (size <= 0) {
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
#if TLS_ENABLE
    event_read(client, read_ssl_data);
#else
    event_read(client, read_header);
#endif
}

int read_preface(event_sock_t *client, int size, uint8_t *buf)
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
        event_write_enable(client, ctx->event_wbuf, HTTP2_MAX_FRAME_SIZE);

#if TLS_ENABLE
        init_tls_server(ctx);
        br_ssl_server_reset(&ctx->sc);

        event_read_start(client, ctx->event_buf, HTTP2_MAX_FRAME_SIZE, read_ssl_handshake);
#else
        event_read_start(client, ctx->event_buf, HTTP2_MAX_FRAME_SIZE, read_preface);
#endif
    }
    else {
        event_close(client, on_client_close);
    }
}

#ifdef CONTIKI
PROCESS_THREAD(tiny_server_process, ev, data)
#else
int main()
#endif
{
#ifdef CONTIKI
    PROCESS_BEGIN();
#endif
    // set client memory
    memset(http2_ctx_list, 0, TINY_MAX_CLIENTS * sizeof(http2_ctx_t));
    for (int i = 0; i < TINY_MAX_CLIENTS - 1; i++) {
        http2_ctx_list[i].next = &http2_ctx_list[i + 1];
    }
    connected = NULL;
    unused = &http2_ctx_list[0];

    event_loop_init(&loop);
    server = event_sock_create(&loop);

#if !defined(CONTIKI) || defined(CONTIKI_TARGET_NATIVE)
    signal(SIGINT, close_server);
#endif

    int r = event_listen(server, 8888, on_new_connection);

    INFO("Starting http/2 server in port 8888");
    if (r < 0) {
        ERROR("Could not start server");
        return 1;
    }
    event_loop(&loop);

#ifdef CONTIKI
    PROCESS_END();
#endif
}
