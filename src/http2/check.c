#include "http2/check.h"
#include "http2/send.h"
#include "http2/utils.h"
#include "logging.h"

int check_incoming_data_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    if (h2s->waiting_for_end_headers_flag) {
        ERROR("CONTINUATION or HEADERS frame was expected. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    if (h2s->current_stream.stream_id == 0) {
        ERROR("Data stream ID is 0. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Data payload bigger than allower. MAX_FRAME_SIZE error");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->header.stream_id > h2s->current_stream.stream_id) {
        ERROR("Stream ID is invalid. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->header.stream_id < h2s->current_stream.stream_id) {
        ERROR("Stream closed. STREAM CLOSED ERROR");
        send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->current_stream.state == STREAM_IDLE) {
        ERROR("Stream was in IDLE state. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->current_stream.state != STREAM_OPEN && h2s->current_stream.state != STREAM_HALF_CLOSED_LOCAL) {
        ERROR("Stream was not in a valid state for data. STREAM CLOSED ERROR");
        send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}


int check_incoming_headers_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    // Check if stream is not created or previous one is closed
    if (h2s->waiting_for_end_headers_flag) {
        //protocol error
        ERROR("CONTINUATION frame was expected. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    if (h2s->header.stream_id == 0) {
        ERROR("Invalid stream id: 0. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    if (h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Frame exceeds the MAX_FRAME_SIZE. FRAME SIZE ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return -1;
    }
    if (h2s->current_stream.state == STREAM_IDLE) {
        if (h2s->header.stream_id < h2s->last_open_stream_id) {
            ERROR("Invalid stream id: not bigger than last open. PROTOCOL ERROR");
            send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
            return -1;
        }
        if (h2s->header.stream_id % 2 != h2s->is_server) {
            INFO("Incoming stream id: %u", h2s->header.stream_id);
            ERROR("Invalid stream id parity. PROTOCOL ERROR");
            send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
            return -1;
        }
        else { // Open a new stream, update last_open and last_peer stream
            h2s->current_stream.stream_id = h2s->header.stream_id;
            h2s->current_stream.state = STREAM_OPEN;
            h2s->last_open_stream_id = h2s->current_stream.stream_id;
            return 0;
        }
    }
    else if (h2s->current_stream.state != STREAM_OPEN &&
             h2s->current_stream.state != STREAM_HALF_CLOSED_LOCAL) {
        //stream closed error
        ERROR("Current stream is not open. STREAM CLOSED ERROR");
        send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
        return -1;
    }
    else if (h2s->header.stream_id != h2s->current_stream.stream_id) {
        //protocol error
        ERROR("Stream ids do not match. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    else {
        return 0;
    }
}

int check_incoming_settings_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    if (h2s->header.stream_id != 0) {
        ERROR("Settings frame stream id is not zero. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    else if (h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Settings payload bigger than allowed. MAX_FRAME_SIZE ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return -1;
    }
    /*Check if ACK is set*/
    if (is_flag_set(h2s->header.flags, SETTINGS_ACK_FLAG)) {
        if (h2s->header.length != 0) {
            ERROR("Settings payload size is not zero. FRAME SIZE ERROR");
            send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
            return -1;
        }
        else {
            if (h2s->wait_setting_ack) {
                h2s->wait_setting_ack = 0;
                return 1;
            }
            else {
                WARN("ACK received but not expected");
                return 1;
            }
        }
    }
    else {
        return 0;
    }
}

int check_incoming_goaway_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    if (h2s->header.stream_id != 0x0) {
        ERROR("GOAWAY doesnt have STREAM ID 0. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    if (h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("GOAWAY payload bigger than allowed. MAX_FRAME_SIZE ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return -1;
    }
    return 0;
}

int check_incoming_continuation_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    if (!h2s->waiting_for_end_headers_flag) {
        ERROR("Continuation must be preceded by a HEADERS frame. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    if (h2s->header.stream_id == 0x0 ||
        h2s->header.stream_id != h2s->current_stream.stream_id) {
        ERROR("Continuation received on invalid stream. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    if (h2s->header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Frame exceeds the MAX_FRAME_SIZE. FRAME SIZE ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return -1;
    }
    if (h2s->current_stream.state == STREAM_IDLE) {
        ERROR("Continuation received on idle stream. PROTOCOL ERROR");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return -1;
    }
    else if (h2s->current_stream.state != STREAM_OPEN) {
        ERROR("Continuation received on closed stream. STREAM CLOSED");
        send_connection_error(buf_out, HTTP2_STREAM_CLOSED, h2s);
        // TODO: send STREAM_CLOSED_ERROR to endpoint.
        return -1;
    }
    if (h2s->header.length >= HTTP2_MAX_HBF_BUFFER - h2s->header_block_fragments_pointer) {
        ERROR("Error block fragments too big (not enough space allocated). INTERNAL ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return -1;
    }
    return 0;
}
