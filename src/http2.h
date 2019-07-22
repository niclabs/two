#ifndef HTTP2_H
#define HTTP2_H
#include "http2utils.h"


/*Default settings values*/
// TODO: use more descriptive names
// i.e. DEFAULT_HEADER_TABLE_SIZE instead of DEFAULT_HTS
#define DEFAULT_HTS 4096
#define DEFAULT_EP 1
#define DEFAULT_MCS 99999
#define DEFAULT_IWS 65535
#define DEFAULT_MFS 16384
#define DEFAULT_MHLS 99999

/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1

/*Error codes*/
typedef enum{
  HTTP2_NO_ERROR = (uint32_t) 0x0,
  HTTP2_PROTOCOL_ERROR = (uint32_t) 0x1,
  HTTP2_INTERNAL_ERROR = (uint32_t) 0x2,
  HTTP2_FLOW_CONTROL_ERROR = (uint32_t) 0x3,
  HTTP2_SETTINGS_TIMEOUT = (uint32_t) 0x4,
  HTTP2_STREAM_CLOSED = (uint32_t) 0x5,
  HTTP2_FRAME_SIZE_ERROR = (uint32_t) 0x6,
  HTTP2_REFUSED_STREAM = (uint32_t) 0x7,
  HTTP2_CANCEL = (uint32_t) 0x8,
  HTTP2_COMPRESSION_ERROR = (uint32_t) 0x9,
  HTTP2_CONNECT_ERROR = (uint32_t) 0xa,
  HTTP2_ENHANCE_YOUR_CALM = (uint32_t) 0xb,
  HTTP2_INADEQUATE_SECURITY = (uint32_t) 0xc,
  HTTP2_HTTP_1_1_REQUIRED = (uint32_t) 0xd
}h2_error_code_t;

/*
* Function: h2_client_init_connection
* Initializes HTTP2 connection between endpoints. Sends preface and local
* settings.
* Input: -> st: pointer to hstates_t struct where variables of client are going
*               to be stored.
* Output: 0 if connection was made successfully. -1 if not.
*/
int h2_client_init_connection(hstates_t *st);

/*
* Function: h2_server_init_connection
* Initializes HTTP2 connection between endpoints. Waits for preface and sends
* local settings.
* Input: -> st: pointer to hstates_t struct where variables of client are going
*               to be stored.
* Output: 0 if connection was made successfully. -1 if not.
*/
int h2_server_init_connection(hstates_t *st);

/*
* Function: h2_receive_frame
* Receives a frame from endpoint, decodes it and works with it.
* Input: -> st: pointer to hstates_t struct where connection variables are stored
* Output: 0 if no problem was found. -1 if error was found.
*/
int h2_receive_frame(hstates_t *st);

/*
* Function: h2_send_request
* Given a set of headers, generates and sends an http2 message to endpoint. The
* message is a request, so it must create a new stream.
* Input: -> st: pointer to hstates_t struct where headers are stored
* Output: 0 if generation and sent was successfull, -1 if not
*/
int h2_send_request(hstates_t *st);

/*
* Function: h2_send_response
* Given a set of headers, generates and sends an http2 message to endpoint. The
* message is a response, so it must be sent through an existent stream, closing
* the current stream.
* Input: -> st: pointer to hstates_t struct where headers are stored
* Output: 0 if generation and sent was successfull, -1 if not
*/
int h2_send_response(hstates_t *st);


/*
* Function: h2_graceful_connection_shutdown
* Sends a GOAWAY FRAME to endpoint with the the last global open stream ID on it
* and a NO ERROR error code. This means that the current process wants to close
* the current connection with endpoint.
* Input: ->st: hstates_t pointer where connection variables are stored
* Output: 0 if no errors were found during goaway sending, -1 if not.
*/

int h2_graceful_connection_shutdown(hstates_t *st);

#endif /*HTTP2_H*/
