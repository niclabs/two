#ifndef HTTP2_H
#define HTTP2_H
#include "http2utils.h"


/*Default settings values*/
#define DEFAULT_HTS 4096
#define DEFAULT_EP 1
#define DEFAULT_MCS 99999
#define DEFAULT_IWS 65535
#define DEFAULT_MFS 16384
#define DEFAULT_MHLS 99999

/*Macros for table update*/
#define LOCAL 0
#define REMOTE 1

/*
* Function: h2_send_local_settings
* Sends local settings to endpoint.
* Input: -> st: pointer to hstates_t struct where local settings are stored
* Output: 0 if settings were sent. -1 if not.
*/
int h2_send_local_settings(hstates_t *hs);

/*
* Function: h2_read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
*        -> st: pointer to hstates_t struct where settings tables are stored.
* Output: The value read from the table. -1 if nothing was read.
*/
uint32_t h2_read_setting_from(uint8_t place, uint8_t param, hstates_t *st);

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

int h2_send_request(hstates_t *st);
int h2_send_response(hstates_t *st);

#endif /*HTTP2_H*/
