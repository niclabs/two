//
// Created by gabriel on 27-01-20.
//

#ifndef TWO_TLS_H
#define TWO_TLS_H
#include "two-conf.h"
#if TLS_ENABLE
#include "event.h"
#include "http2.h"
void init_tls_server(http2_context_t *ctx);

void write_ssl_handshake(event_sock_t *client, int status);
void write_ssl(event_sock_t *client, int status);
int read_ssl_data(event_sock_t *client, int size, uint8_t *buf);
int send_app_ssl_data(event_sock_t *client, int size, uint8_t *buf);
int read_ssl_handshake(event_sock_t *client, int size, uint8_t *buf);
#endif //TLS_ENABLE
#endif //TWO_TLS_H
