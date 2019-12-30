#include <string.h>

#include "http2/http2.h"
#include "http2/check.h"
#include "http2/send.h"
#include "http2/utils.h"
#include "http2/flowcontrol.h"
#include "http2/handle.h"
#include "frames.h"
#include "http.h"

// Specify to which module this file belongs
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

extern void two_on_client_close(event_sock_t *client);

int exchange_prefaces(event_sock_t * client, int size, uint8_t *bytes);
int receive_header(event_sock_t * client, int size, uint8_t *bytes);
int receive_payload(event_sock_t * client, int size, uint8_t *bytes);
int receive_payload_wait_settings_ack(event_sock_t * client, int size, uint8_t *bytes);
int receive_payload_goaway(event_sock_t * client, int size, uint8_t *bytes);
int check_incoming_condition(h2states_t *h2s);
int handle_payload(uint8_t *buff_read, h2states_t *h2s);

int init_variables_h2s(h2states_t *h2s, uint8_t is_server, event_sock_t *socket)
{
    h2s->socket = socket;
    h2s->is_server = is_server;
    #if HPACK_INCLUDE_DYNAMIC_TABLE
    h2s->remote_settings[0] = h2s->local_settings[0] = DEFAULT_HEADER_TABLE_SIZE;
    #else
    h2s->remote_settings[0] = h2s->local_settings[0] = 0;
    #endif
    h2s->remote_settings[1] = h2s->local_settings[1] = DEFAULT_ENABLE_PUSH;
    h2s->remote_settings[2] = h2s->local_settings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
    h2s->remote_settings[3] = h2s->local_settings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->remote_settings[4] = h2s->local_settings[4] = DEFAULT_MAX_FRAME_SIZE;
    h2s->remote_settings[5] = h2s->local_settings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
    h2s->wait_setting_ack = 0;
    h2s->current_stream.stream_id = is_server ? 2 : 3;
    h2s->current_stream.state = STREAM_IDLE;
    h2s->last_open_stream_id = 0;
    h2s->header_block_fragments_pointer = 0;
    h2s->waiting_for_end_headers_flag = 0;
    h2s->waiting_for_HEADERS_frame = 0;
    h2s->received_end_stream = 0;
    h2s->write_callback_is_set = 0;
    h2s->remote_window.connection_window = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->remote_window.stream_window = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->local_window.connection_window = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->local_window.stream_window = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->sent_goaway = 0;
    h2s->received_goaway = 0;
    h2s->debug_size = 0;
    //h2s->header = NULL;
    hpack_init_states(&(h2s->hpack_states), DEFAULT_HEADER_TABLE_SIZE);

    headers_init(&(h2s->headers));

    h2s->data.buf = h2s->header_block_fragments;

    return HTTP2_RC_NO_ERROR;
}

void clean_h2s(event_sock_t *client)
{
    // release the client from the list
    two_on_client_close(client);
}

int on_timeout(event_sock_t * sock) {
    event_close(sock, clean_h2s);
    return 1;
}

void http2_server_init_connection(event_sock_t *client, int status)
{
    (void)status;
    // Cast h2states from state
    h2states_t *h2s = (h2states_t *)client->data;

    // Initialize http2 state
    init_variables_h2s(h2s, 1, client);

    event_timeout(client, 500, on_timeout);
    event_read_start(client, h2s->input_buf, HTTP2_MAX_BUFFER_SIZE, exchange_prefaces);
}

void http2_on_read_continue(event_sock_t *client, int status)
{
    if (status < 0) {
        event_close(client, clean_h2s);
    }
    else {
        event_read(client, receive_header);
    }
}

int exchange_prefaces(event_sock_t * client, int size, uint8_t *bytes)
{
    // Cast h2states from state
    h2states_t *h2s = (h2states_t *)client->data;

    int bytes_read = 0;

    if (size <= 0) {
        INFO("Client's socket disconnected");
        event_close(client, clean_h2s);
        return bytes_read;
    }

    if (size < 24) {
        return bytes_read;
    }

    bytes_read = 24;

    if (strncmp((char*)bytes, "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n", 24) != 0) {
        ERROR("Bad preface (%.5s) received from client", bytes);
        send_connection_error(HTTP2_PROTOCOL_ERROR, h2s);
        event_close(h2s->socket, clean_h2s);
        DEBUG("http2_server_init_connection returning null callback");
        return bytes_read;
    }
    DEBUG("Received HTTP/2 connection preface. Sending local settings");

    // Send local settings
    int rc = send_local_settings(h2s);
    if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
        DEBUG("Error sending local settings in http2_server_init_connection");
        return bytes_read;
    }

    h2s->waiting_for_HEADERS_frame = 1;
    DEBUG("Local settings sent. http2_server_init_connection returning receive_header callback");

    // If no errors were found, http2 is ready to receive frames
    return bytes_read;
}

int receive_header(event_sock_t * client, int size, uint8_t *bytes)
{
    // Cast h2states from state
    h2states_t *h2s = (h2states_t *)client->data;
    h2s->write_callback_is_set = 0;
    int bytes_read = 0;

    if (size <= 0) {
        INFO("Client's socket disconnected");
        event_close(client, clean_h2s);
        return bytes_read;
    }

    // Wait until header length is received
    if (size < 9) {
        DEBUG("http2_receive_header didn't receive anough bytes, received %d out of 9", size);
        return bytes_read;
    }

    // Read header data from input buffer and decode it
    frame_header_t header;
    int rc = frame_header_from_bytes(bytes, 9, &header);
    bytes_read = 9;

    if (rc && rc != FRAMES_PROTOCOL_ERROR && rc != FRAMES_FRAME_SIZE_ERROR && rc != FRAMES_FRAME_NOT_FOUND_ERROR) {
        ERROR("Failed to decode frame header. Sending INTERNAL_ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        event_close(h2s->socket, clean_h2s);

        DEBUG("Internal error response sent. Terminating connection");
        return bytes_read;
    }
    else if (rc == FRAMES_PROTOCOL_ERROR) {
        ERROR("Failed to decode frame header. Sending PROTOCOL_ERROR");
        send_connection_error(HTTP2_PROTOCOL_ERROR, h2s);
        event_close(h2s->socket, clean_h2s);

        DEBUG("Protocol error response sent. Terminating connection");
        return bytes_read;
    }
    else if (rc == FRAMES_FRAME_SIZE_ERROR) {
        ERROR("Failed to decode frame header. Sending FRAME_SIZE_ERROR");
        send_connection_error(HTTP2_FRAME_SIZE_ERROR, h2s);
        event_close(h2s->socket, clean_h2s);

        DEBUG("Frame size error response sent. Terminating connection");
        return bytes_read;
    }
    DEBUG("Received new frame header 0x%d", header.type);

    // Reject frames with payload bigger than the read buffer
    if(header.length > HTTP2_MAX_BUFFER_SIZE){
      ERROR("Frame with payload bigger than reading buffer");
      send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
      event_close(h2s->socket, clean_h2s);
      return bytes_read;
    }

    // Read max frame size from local settings
    uint32_t local_max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
    if (header.length > local_max_frame_size) {
        ERROR("Invalid received frame size %d > %d. Sending FRAME_SIZE_ERROR", header.length, local_max_frame_size);
        send_connection_error(HTTP2_FRAME_SIZE_ERROR, h2s);
        event_close(h2s->socket, clean_h2s);

        DEBUG("FRAME_SIZE_ERROR sent. Terminating connection");
        DEBUG("http2_receive_header returning null callback");
        return bytes_read;
    }

    // save header type
    h2s->header = header;

    if (rc == FRAMES_FRAME_NOT_FOUND_ERROR) {
        if (h2s->waiting_for_end_headers_flag) {
            ERROR("Unknown/extension frame type in the middle of a header block. PROTOCOL_ERROR");
            send_connection_error(HTTP2_PROTOCOL_ERROR, h2s);
            event_close(client, clean_h2s);
            return bytes_read;
        }
        event_read(client, receive_payload);
        return bytes_read;
    }

    // If errors are found, internal logic will handle them.
    // TODO: improve method names, it is not clear on reading the code
    // what are the conditions checked
    rc = check_incoming_condition(h2s);
    if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT || rc == HTTP2_RC_ERROR) {
        DEBUG("check conditions for incoming headers returned error");
        event_close(client, clean_h2s);
        return bytes_read;
    }
    else if (rc == HTTP2_RC_ACK_RECEIVED) {
        event_read(client, receive_header);
        DEBUG("incoming_condition returned HTTP2_RC_ACK_RECEIVED");
        DEBUG("http2_receive_header returning receive_header callback");
        return bytes_read;
    }
    DEBUG("http2_receive_header returning receive_payload callback");

    if (!h2s->write_callback_is_set) {
        event_read(client, receive_payload);
    }
    return bytes_read;
}

int receive_payload(event_sock_t * client, int size, uint8_t *bytes)
{
    // Cast h2states from state
    h2states_t *h2s = (h2states_t *)client->data;

    int bytes_read = 0;

    if (size <= 0) {
        INFO("Client's socket disconnected");
        event_close(client, clean_h2s);
        return bytes_read;
    }

    // Wait until all payload data has been received
    if (size < h2s->header.length) {
        DEBUG("http2_receive_payload didn't receive enough bytes, received %d out of %d", size, h2s->header.length);
        return bytes_read;
    }

    bytes_read = h2s->header.length;

    // Process payload
    int rc = handle_payload(bytes, h2s);
    if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT || rc == HTTP2_RC_ERROR || rc == HTTP2_RC_CLOSE_CONNECTION) {
        DEBUG("http2_receive_payload returning null callback");
        event_close(h2s->socket, clean_h2s);
        return bytes_read;
    }

    // Wait for next header
    DEBUG("http2_receive_payload returning receive_header callback");
    if (!h2s->write_callback_is_set) {
        event_read(client, receive_header);
    }
    return bytes_read;
}



int check_incoming_condition(h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            rc = check_incoming_data_condition(h2s);
            return rc;
        }
        case HEADERS_TYPE: {
            rc = check_incoming_headers_condition(h2s);
            return rc;
        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case SETTINGS_TYPE: {
            rc = check_incoming_settings_condition(h2s);
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case PING_TYPE: {//Ping
            rc = check_incoming_ping_condition(h2s);
            return rc;
        }
        case GOAWAY_TYPE: {
            rc = check_incoming_goaway_condition(h2s);
            return rc;
        }
        case WINDOW_UPDATE_TYPE: {
            rc = check_incoming_window_update_condition(h2s);
            return rc;
        }
        case CONTINUATION_TYPE: {
            rc = check_incoming_continuation_condition(h2s);
            return rc;
        }
        default: {
            return HTTP2_RC_NO_ERROR;
        }
    }
}

/*
 *
 *
 *
 */
int handle_payload(uint8_t *buff_read, h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            DEBUG("handle_payload: RECEIVED DATA PAYLOAD");

            data_payload_t data_payload;
            uint8_t data[h2s->header.length];
            data_payload.data = data;

            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &data_payload, buff_read);
            if (rc < 0) {
                ERROR("ERROR reading data payload");
                send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }

            rc = handle_data_payload(&(h2s->header), &data_payload, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("ERROR in handle receive data.");
                return rc;
            }
            else if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                DEBUG("Close connection. GOAWAY received and sent");
                return rc;
            }

            DEBUG("handle_payload: RECEIVED DATA PAYLOAD OK");
            return HTTP2_RC_NO_ERROR;
        }
        case HEADERS_TYPE: {
            DEBUG("handle_payload: RECEIVED HEADERS PAYLOAD");

            headers_payload_t hpl;
            uint8_t headers_block_fragment[h2s->header.length];
            uint8_t padding[32];
            hpl.header_block_fragment = headers_block_fragment;
            hpl.padding = padding;

            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &hpl, buff_read);
            if (rc < 0) {
                ERROR("ERROR reading headers payload");
                send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            rc = handle_headers_payload(&(h2s->header), &hpl, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("ERROR in handle receive headers.");
                return rc;
            }
            else if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                DEBUG("Close connection. GOAWAY received and sent");
                return rc;
            }
            DEBUG("handle_payload: RECEIVED HEADERS PAYLOAD OK");
            return HTTP2_RC_NO_ERROR;

        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case SETTINGS_TYPE: {
            DEBUG("Received SETTINGS payload");

            settings_payload_t spl;
            settings_pair_t pairs[h2s->header.length / 6];
            spl.pairs = pairs;

            // Decode settings payload
            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &spl, buff_read);
            if (rc < 0) {
                // bytes_to_settings_payload returns -1 if length is not a multiple of 6. RFC 6.5
                ERROR("ERROR: SETTINGS frame with a length other than a multiple of 6 octets. FRAME_SIZE_ERROR");
                send_connection_error(HTTP2_FRAME_SIZE_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }

            // Use remote settings
            rc = handle_settings_payload(&spl, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("ERROR in handle receive settings payload.");
                return rc;
            }
            DEBUG("SETTINGS payload received OK");
            rc = send_try_continue_data_sending(h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("Error trying to send data");
                return rc;
            }
            return HTTP2_RC_NO_ERROR;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return HTTP2_RC_ERROR;

        case PING_TYPE://Ping
            DEBUG("handle_payload: RECEIVED PING PAYLOAD");

            ping_payload_t ping_payload;
            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &ping_payload, buff_read);
            if (rc < 0) {
                ERROR("ERROR reading ping payload");
                send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            rc = handle_ping_payload(&ping_payload, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("ERROR in handle receive ping");
                return rc;
            }
            DEBUG("handle_payload: RECEIVED PING PAYLOAD OK");
            return HTTP2_RC_NO_ERROR;

        case GOAWAY_TYPE: {
            DEBUG("handle_payload: RECEIVED GOAWAY PAYLOAD");

            uint32_t max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
            uint8_t debug_data[max_frame_size - 8];
            goaway_payload_t goaway_pl;
            goaway_pl.additional_debug_data = debug_data;

            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &goaway_pl, buff_read);
            if (rc < 0) {
                ERROR("Error in reading goaway payload");
                send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            rc = handle_goaway_payload(&goaway_pl, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                INFO("Received GOAWAY. Close Connection");
                return rc;
            }
            else if (rc == HTTP2_RC_ERROR){
                ERROR("ERROR while receiving goaway");
                return rc;
            }
            DEBUG("handle_payload: RECEIVED GOAWAY PAYLOAD OK");
            return HTTP2_RC_NO_ERROR;
        }
        case WINDOW_UPDATE_TYPE: {
            DEBUG("handle_payload: RECEIVED WINDOW_UPDATE PAYLOAD");
            window_update_payload_t window_update_payload;
            rc= h2s->header.callback_payload_from_bytes(&(h2s->header), &window_update_payload, buff_read);
            if (rc < 0) {
                ERROR("Error in reading window_update_payload. FRAME_SIZE_ERROR");
                send_connection_error(HTTP2_FRAME_SIZE_ERROR, h2s); // TODO: review - always FRAME_SIZE_ERROR ?
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            rc = handle_window_update_payload(&window_update_payload, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("Error during window_update handling");
                return rc;
            }
            DEBUG("handle_payload: RECEIVED WINDOW_UPDATE PAYLOAD OK");
            rc = send_try_continue_data_sending(h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("Error trying to send data");
                return rc;
            }
            return HTTP2_RC_NO_ERROR;
        }
        case CONTINUATION_TYPE: {
            DEBUG("handle_payload: RECEIVED CONTINUATION PAYLOAD");

            continuation_payload_t contpl;
            uint8_t continuation_block_fragment[HTTP2_MAX_HBF_BUFFER - h2s->header_block_fragments_pointer];
            contpl.header_block_fragment = continuation_block_fragment;

            rc = h2s->header.callback_payload_from_bytes(&(h2s->header), &contpl, buff_read);
            if (rc < 0) {
                ERROR("Error in continuation payload reading");
                send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }

            rc = handle_continuation_payload(&h2s->header, &contpl, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("ERROR in handle receive continuation payload.");
                return rc;
            }
            else if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                DEBUG("Close connection. GOAWAY received and sent");
                return rc;
            }
            DEBUG("handle_payload: RECEIVED CONTINUATION PAYLOAD OK");
            return HTTP2_RC_NO_ERROR;
        }
        default: {
            WARN("handle_payload: IGNORING UNKNOWN PAYLOAD");
            return HTTP2_RC_NO_ERROR;
        }
    }
}
