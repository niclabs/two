#include "http2/flowcontrol.h"
#include "http2/handle.h"
#include "http2/utils.h"
#include "http2/structs.h"
#include "frames.h"
#include "http2/send.h"
#include "http.h"
#include "string.h"

// Specify to which module this file belongs
#include "config.h"
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

int validate_pseudoheaders(header_list_t *pseudoheaders)
{

    if (headers_get(pseudoheaders, ":method") == NULL) {
        ERROR("\":method\" pseudoheader was missing.");
        return HTTP2_RC_ERROR;
    }

    if (headers_get(pseudoheaders, ":scheme") == NULL) {
        ERROR("\":scheme\" pseudoheader was missing.");
        return HTTP2_RC_ERROR;
    }

    if (headers_get(pseudoheaders, ":path") == NULL) {
        ERROR("\":path\" pseudoheader was missing.");
        return HTTP2_RC_ERROR;
    }

    return (h2_ret_code_t)headers_validate(pseudoheaders);
}

/*
 *
 *
 *
 */
int handle_data_payload(frame_header_t *frame_header, data_payload_t *data_payload, cbuf_t *buf_out, h2states_t *h2s)
{
    uint32_t data_length = frame_header->length;//padding not implemented(-data_payload->pad_length-1 if pad_flag_set)
    /*check flow control*/
    int rc = flow_control_receive_data(h2s, data_length);

    if (rc < 0) {
        send_connection_error(buf_out, HTTP2_FLOW_CONTROL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    memcpy(h2s->data.buf + h2s->data.size, data_payload->data, data_length);
    h2s->data.size += data_length;
    if (is_flag_set(frame_header->flags, DATA_END_STREAM_FLAG)) {
        h2s->received_end_stream = 1;
    }
    // Stream state handling for end stream flag
    if (h2s->received_end_stream == 1) {
        rc = change_stream_state_end_stream_flag(0, buf_out, h2s); // 0 is for receiving
        if (rc == 2) {
            DEBUG("handle_data_payload: Close connection. GOAWAY previously received");
            return HTTP2_RC_CLOSE_CONNECTION;
        }
        h2s->waiting_for_HEADERS_frame = 1;
        h2s->received_end_stream = 0;
        rc = validate_pseudoheaders(&h2s->headers);
        if (rc < 0) {
            ERROR("handle_data_payload: Malformed request received");
            send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }
        // Generate response through http layer.
        rc = http_server_response(h2s->data.buf, &h2s->data.size, &h2s->headers);
        if (rc < 0) {
            DEBUG("An error occurred during http layer response generation");
            send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }

        // Generate http2 response using http response.
        return (h2_ret_code_t)send_response(buf_out, h2s);
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: handle_headers_payload
 * Does all the operations related to an incoming HEADERS FRAME.
 * Input: -> header: pointer to the headers frame header (frame_header_t)
 *        -> hpl: pointer to the headers payload (headers_payload_t)
 *        -> st: pointer to h2states_t struct where connection variables are stored
 * Output: 0 if no error was found, -1 if not.
 */
int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, cbuf_t *buf_out, h2states_t *h2s)
{
    // we receive a headers, so it could be continuation frames
    int rc;

    h2s->waiting_for_end_headers_flag = 1;
    int hbf_size = get_header_block_fragment_size(header, hpl);
    // We are reading a new header frame, so previous fragments are useless
    if (h2s->header_block_fragments_pointer != 0) {
        h2s->header_block_fragments_pointer = 0;
    }
    // We check if hbf fits on buffer
    if (hbf_size >= HTTP2_MAX_HBF_BUFFER) {
        ERROR("Header block fragments too big (not enough space allocated). INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    //first we receive fragments, so we save those on the h2s->header_block_fragments buffer
    //rc =
    memcpy(h2s->header_block_fragments + h2s->header_block_fragments_pointer, hpl->header_block_fragment, hbf_size);
    /*if (rc < 1) {
        ERROR("Headers' header block fragment were not written or paylaod was empty. rc = %d", rc);
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
       }*/
    h2s->header_block_fragments_pointer += hbf_size;
    //If end_stream is received-> wait for an end headers (in header or continuation) to half_close the stream
    if (is_flag_set(header->flags, HEADERS_END_STREAM_FLAG)) {
        h2s->received_end_stream = 1;
    }
    //when receive (continuation or header) frame with flag end_header then the fragments can be decoded, and the headers can be obtained.
    if (is_flag_set(header->flags, HEADERS_END_HEADERS_FLAG)) {
        //return bytes read.
        rc = receive_header_block(h2s->header_block_fragments, h2s->header_block_fragments_pointer, &h2s->headers, &h2s->hpack_states);
        if (rc < 0) {
            if (rc == -1) {
                ERROR("Error was found receiving header_block. COMPRESSION ERROR");
                send_connection_error(buf_out, HTTP2_COMPRESSION_ERROR, h2s);
            }
            else if (rc == -2) {
                ERROR("Error was found receiving header_block. INTERNAL ERROR");
                send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
            }
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }
        if (rc != h2s->header_block_fragments_pointer) {
            ERROR("ERROR still exists fragments to receive. Read %d bytes of %d bytes", rc, h2s->header_block_fragments_pointer);
            send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }
        else {//all fragments already received.
            h2s->header_block_fragments_pointer = 0;
        }
        h2s->waiting_for_end_headers_flag = 0;                          //RESET TO 0
        if (h2s->received_end_stream == 1) {
            rc = change_stream_state_end_stream_flag(0, buf_out, h2s);  //0 is for receiving
            if (rc == 2) {
                DEBUG("handle_headers_payload: Close connection. GOAWAY previously received");
                return HTTP2_RC_CLOSE_CONNECTION;
            }
            h2s->waiting_for_HEADERS_frame = 1;
            h2s->received_end_stream = 0;//RESET TO 0
            rc = validate_pseudoheaders(&h2s->headers);
            if (rc < 0) {
                ERROR("handle_headers_payload: Malformed request received");
                send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            // Generate response through http layer
            rc = http_server_response(h2s->data.buf, &h2s->data.size, &h2s->headers);
            if (rc < 0) {
                DEBUG("An error occurred during http layer response generation");
                send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }

            // Generate http2 response using http response
            return (h2_ret_code_t)send_response(buf_out, h2s);
        }
        uint32_t header_list_size = headers_get_header_list_size(&h2s->headers);
        uint32_t setting_read = read_setting_from(h2s, LOCAL, MAX_HEADER_LIST_SIZE);
        uint32_t MAX_HEADER_LIST_SIZE_VALUE = (uint32_t)setting_read;
        if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
            ERROR("Header list size greater than max allowed. Send HTTP 431");
            //TODO send error and finish stream
            return HTTP2_RC_NO_ERROR;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: update_settings_table
 * Updates the specified table of settings with a given settings payload.
 * Input: -> spl: settings_payload_t pointer where settings values are stored
 *        -> place: must be LOCAL or REMOTE. It indicates which table to update.
   -> st: pointer to h2states_t struct where settings table are stored.
 * Output: 0 if update was successfull, -1 if not
 */
int update_settings_table(settings_payload_t *spl, uint8_t place, cbuf_t *buf_out, h2states_t *h2s)
{
    uint8_t i;

    for (i = 0; i < spl->count; i++) {
        uint16_t id;
        uint32_t value;
        id = spl->pairs[i].identifier;
        value = spl->pairs[i].value;
        if (id < 1 || id > 6) {
            continue;
        }
        switch (id) {
            case ENABLE_PUSH:
                if (value != 0 && value != 1) {
                    ERROR("Invalid value in ENABLE_PUSH settings. Protocol Error");
                    send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
                    return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
                }
                break;
            case INITIAL_WINDOW_SIZE:
                if (value > 2147483647) {
                    ERROR("Invalid value in INITIAL_WINDOW_SIZE settings. Protocol Error");
                    send_connection_error(buf_out, HTTP2_FLOW_CONTROL_ERROR, h2s);
                    return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
                }
                int rc = update_window_size(h2s, value, place);
                if (rc == HTTP2_RC_ERROR) {
                    ERROR("Change in SETTINGS_INITIAL_WINDOW_SIZE caused a window to exceed the maximum size. FLOW_CONTROL_ERROR");
                    send_connection_error(buf_out, HTTP2_FLOW_CONTROL_ERROR, h2s);
                    return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
                }
                break;
            case MAX_FRAME_SIZE:
                if (value > 16777215 || value < 16384) {
                    ERROR("Invalid value in MAX_FRAME_SIZE settings. Protocol Error");
                    send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
                    return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
                }
                break;
            default:
                break;
        }
        if (place == REMOTE) {
            h2s->remote_settings[--id] = spl->pairs[i].value;
        }
        else if (place == LOCAL) {
            h2s->local_settings[--id] = spl->pairs[i].value;
        }
        else {
            WARN("Invalid table");
            break;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: handle_settings_payload
 * Reads a settings payload from buffer and works with it.
 * Input: -> buff_read: buffer where payload's data is written
        -> header: pointer to a frameheader_t structure already built with frame info
        -> spl: pointer to settings_payload_t struct where data is gonna be written
        -> pairs: pointer to settings_pair_t array where data is gonna be written
        -> st: pointer to h2states_t struct where connection variables are stored
 * Output: 0 if operations are done successfully, -1 if not.
 */
int handle_settings_payload(settings_payload_t *spl, cbuf_t *buf_out, h2states_t *h2s)
{
    // update_settings_table checks for possible errors in the incoming settings
    if (!update_settings_table(spl, REMOTE, buf_out, h2s)) {
        int rc = send_settings_ack(buf_out, h2s);
        return (h2_ret_code_t)rc;
    }
    else {
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
}

/*
 * Function: handle_goaway_payload
 * Handles go away payload.
 * Input: ->goaway_pl: goaway_payload_t pointer to goaway frame payload
 *        ->st: pointer h2states_t struct where connection variables are stored
 * IMPORTANT: this implementation doesn't check the correctness of the last stream
 * id sent by the endpoint. That is, if a previous GOAWAY was received with an n
 * last_stream_id, it assumes that the next value received is going to be the same.
 * Output: 0 if no error were found during the handling, 1 if not
 */
int handle_goaway_payload(goaway_payload_t *goaway_pl, cbuf_t *buf_out, h2states_t *h2s)
{
    if (goaway_pl->error_code != HTTP2_NO_ERROR) {
        INFO("Received GOAWAY with ERROR");
        // i guess that is closed on the other side, are you?
        return HTTP2_RC_CLOSE_CONNECTION;
    }
    if (h2s->sent_goaway == 1) { // received answer to goaway
        INFO("Connection CLOSED");
        // Return -1 to close connection
        return HTTP2_RC_CLOSE_CONNECTION;
    }
    if (h2s->received_goaway == 1) {
        INFO("Another GOAWAY has been received before, just info");
    }
    else {                          // never has been seen a goaway before in this connection life
        h2s->received_goaway = 1;   // receiver must not open additional streams
    }
    if (h2s->current_stream.stream_id > goaway_pl->last_stream_id) {
        if (h2s->current_stream.state != STREAM_IDLE) {
            h2s->current_stream.state = STREAM_CLOSED;
            INFO("Current stream closed");
        }
        int rc = send_goaway(HTTP2_NO_ERROR, buf_out, h2s); // We send a goaway to close the connection
        // TODO: review error code from send_goaway in handle_goaway_payload
        if (rc < 0) {
            ERROR("Error sending GOAWAY FRAME");            // TODO shutdown_connection
            return HTTP2_RC_CLOSE_CONNECTION;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: handle_ping_payload
 * Handles ping payload.
 * Input: -> ping_payload: payload of ping frame.
 *        -> h2s: pointer h2states_t struct where connection variables are stored
 * IMPORTANT: this implementation doesn't check the correctness of the last stream
 * Output: HTTP2_RC_NO_ERROR if no error were found during the handling.
 */
int handle_ping_payload(ping_payload_t *ping_payload, cbuf_t *buf_out, h2states_t *h2s)
{
    if (h2s->header.flags == PING_ACK_FLAG) { // received ACK to PING
        INFO("Received ACK to PING frame");
        return HTTP2_RC_NO_ERROR;
    }
    else {                                                                      //Received PING frame with no ACK
        int8_t ack_flag = 1;                                                    //TRUE
        int rc = send_ping(ping_payload->opaque_data, ack_flag, buf_out, h2s);  //Response a PING ACK frame
        return (h2_ret_code_t)rc;
    }
}

/*
 * Function: handle_continuation_payload
 * Does all the operations related to an incoming CONTINUATION FRAME.
 * Input: -> header: pointer to the continuation frame header (frame_header_t)
 *        -> hpl: pointer to the continuation payload (continuation_payload_t)
 *        -> st: pointer to h2states_t struct where connection variables are stored
 * Output: 0 if no error was found, -1 if not.
 */
int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, cbuf_t *buf_out, h2states_t *h2s)
{
    int rc;

    //We check if payload fits on buffer
    if (header->length >= HTTP2_MAX_HBF_BUFFER - h2s->header_block_fragments_pointer) {
        ERROR("Continuation Header block fragments doesnt fit on buffer (not enough space allocated). INTERNAL ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    //receive fragments and save those on the h2s->header_block_fragments buffer
    //rc =
    memcpy(h2s->header_block_fragments + h2s->header_block_fragments_pointer, contpl->header_block_fragment, header->length);
    /*if (rc < 1) {
        ERROR("Continuation block fragment was not written or payload was empty");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
       }*/
    h2s->header_block_fragments_pointer += header->length;
    if (is_flag_set(header->flags, CONTINUATION_END_HEADERS_FLAG)) {
        //return number of headers written on header_list, so http2 can update header_list_count
        rc = receive_header_block(h2s->header_block_fragments, h2s->header_block_fragments_pointer, &h2s->headers, &h2s->hpack_states); //TODO check this: rc is the byte read from the header

        if (rc < 0) {
            if (rc == -1) {
                ERROR("Error was found receiving header_block. COMPRESSION ERROR");
                send_connection_error(buf_out, HTTP2_COMPRESSION_ERROR, h2s);
            }
            else if (rc == -2) {
                ERROR("Error was found receiving header_block. INTERNAL ERROR");
                send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
            }
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }
        if (rc != h2s->header_block_fragments_pointer) {
            ERROR("ERROR still exists fragments to receive.");
            send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
            return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
        }
        else {//all fragments already received.
            h2s->header_block_fragments_pointer = 0;
        }
        //st->hd_lists.header_list_count_in = rc;
        h2s->waiting_for_end_headers_flag = 0;
        if (h2s->received_end_stream == 1) {                            //IF RECEIVED END_STREAM IN HEADER FRAME, THEN CLOSE THE STREAM
            rc = change_stream_state_end_stream_flag(0, buf_out, h2s);  //0 is for receiving
            if (rc == 2) {
                DEBUG("handle_continuation_payload: Close connection. GOAWAY previously received");
                return HTTP2_RC_CLOSE_CONNECTION;
            }
            h2s->waiting_for_HEADERS_frame = 1;
            h2s->received_end_stream = 0;       //RESET TO 0
            rc = validate_pseudoheaders(&h2s->headers);
            if (rc < 0) {
                ERROR("handle_continuation_payload: Malformed request received");
                send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }
            // Generate response through http layer
            rc = http_server_response(h2s->data.buf, &h2s->data.size, &h2s->headers);
            if (rc < 0) {
                DEBUG("An error occurred during http layer response generation");
                send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
                return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
            }

            // Generate http2 response using http response
            return (h2_ret_code_t)send_response(buf_out, h2s);
        }
        uint32_t header_list_size = headers_get_header_list_size(&h2s->headers);
        uint32_t setting_read = read_setting_from(h2s, LOCAL, MAX_HEADER_LIST_SIZE);
        uint32_t MAX_HEADER_LIST_SIZE_VALUE = (uint32_t)setting_read;
        if (header_list_size > MAX_HEADER_LIST_SIZE_VALUE) {
            WARN("Header list size greater than max allowed. Send HTTP 431");
            //TODO send error and finish stream
            return HTTP2_RC_NO_ERROR;
        }

    }
    return HTTP2_RC_NO_ERROR;
}


int handle_window_update_payload(window_update_payload_t *wupl, cbuf_t *buf_out, h2states_t *h2s)
{
    uint32_t window_size_increment = wupl->window_size_increment;

    if (window_size_increment == 0) {
        ERROR("Flow-control window increment is 0. Stream Error. PROTOCOL_ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    int rc = flow_control_receive_window_update(h2s, window_size_increment);
    if (rc < 0) {
        send_connection_error(buf_out, HTTP2_FLOW_CONTROL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}
