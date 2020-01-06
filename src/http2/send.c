#include "http2/send.h"
#include "frames.h"
#include "headers.h"
#include "http2/utils.h"
#include "http2/flowcontrol.h"

// Specify to which module this file belongs
#include "config.h"
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

extern void http2_on_read_continue(event_sock_t *client, int status);

/*
 * Function: change_stream_state_end_stream_flag
 * Given an h2states_t struct and a boolean, change the state of the current stream
 * when a END_STREAM_FLAG is sent or received.
 * Input: ->st: pointer to h2states_t struct where connection variables are stored
 *        ->sending: boolean like uint8_t that indicates if current flag is sent or received
 * Output: -1 if connection must be closed, 0 otherwise
 */
int change_stream_state_end_stream_flag(uint8_t sending, h2states_t *h2s)
{
    if (sending) { // Change stream status if end stream flag is sending
        if (h2s->current_stream.state == STREAM_OPEN) {
            h2s->current_stream.state = STREAM_HALF_CLOSED_LOCAL;
        }
        else if (h2s->current_stream.state == STREAM_HALF_CLOSED_REMOTE) {
            h2s->current_stream.state = STREAM_CLOSED;
            if (FLAG_VALUE(h2s->flag_bits, FLAG_RECEIVED_GOAWAY)) {
                send_goaway(HTTP2_NO_ERROR, h2s);
                DEBUG("change_stream_state_end_stream_flag: Close connection. GOAWAY frame was previously received");
                return HTTP2_RC_CLOSE_CONNECTION;
            }
            else {
                prepare_new_stream(h2s);
            }
        }
    }
    else { // Change stream status if send stream flag is received
        if (h2s->current_stream.state == STREAM_OPEN) {
            h2s->current_stream.state = STREAM_HALF_CLOSED_REMOTE;
        }
        else if (h2s->current_stream.state == STREAM_HALF_CLOSED_LOCAL) {
            h2s->current_stream.state = STREAM_CLOSED;
            if (FLAG_VALUE(h2s->flag_bits, FLAG_RECEIVED_GOAWAY)) {
                send_goaway(HTTP2_NO_ERROR, h2s);
                DEBUG("change_stream_state_end_stream_flag: Close connection. GOAWAY frame was previously received");
                return HTTP2_RC_CLOSE_CONNECTION;
            }
            else {
                prepare_new_stream(h2s);
            }
        }
    }
    return HTTP2_RC_NO_ERROR;
}

int send_data_frame(uint32_t data_to_send, uint8_t end_stream, h2states_t *h2s)
{
    uint32_t stream_id = h2s->current_stream.stream_id;
    frame_t frame;
    frame_header_t frame_header;
    data_payload_t data_payload;
    uint8_t data[data_to_send];

    create_data_frame(&frame_header, &data_payload, data, h2s->data.buf + h2s->data.processed, data_to_send, stream_id);

    if (end_stream) {
        frame_header.flags = set_flag(frame_header.flags, DATA_END_STREAM_FLAG);
    }
    frame.frame_header = &frame_header;
    frame.payload = (void *)&data_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    INFO("Sending DATA");
    int rc;
    if (end_stream) {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_bytes, http2_on_read_continue);
        SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    }
    else {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_bytes, NULL);
    }
    if (rc != bytes_size) {
        ERROR("send_data: Error writing data frame. Couldn't push %d bytes to buffer. INTERNAL ERROR", rc);
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: send_data
 * Sends a data frame with the current data written in the given hstates_t struct.
 * The data is expected to be written in the http_data_t.data_out.buf, and its
 * corresponding size indicated in the http_data_t.data_out.size variable.
 * Input: ->st: hstates_t struct where connection variables are stored
         ->end_stream: indicates if the data frame to be sent must have the
                      END_STREAM_FLAG set.
 * Output: 0 if no error were found in the process, -1 if not
 *
 */
int send_data(uint8_t end_stream, h2states_t *h2s)
{
    (void) end_stream;
    if (h2s->data.size <= 0) {
        ERROR("No data to be sent. INTERNAL_ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    h2_stream_state_t state = h2s->current_stream.state;
    if (state != STREAM_OPEN && state != STREAM_HALF_CLOSED_REMOTE) {
        ERROR("Wrong state for sending DATA. INTERNAL_ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    int rc;
    // Check available window
    uint32_t available_data_to_send = get_sending_available_window(h2s);
    uint32_t data_to_send = h2s->data.size - h2s->data.processed;
    uint32_t max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
    // There is enough window to send all
    if (data_to_send <= available_data_to_send) {
        available_data_to_send = data_to_send;
        if (data_to_send > max_frame_size) {
            while (data_to_send > max_frame_size) {
                rc = send_data_frame(max_frame_size, 0, h2s);
                if (rc != HTTP2_RC_NO_ERROR) {
                    return rc;
                }
                data_to_send -= max_frame_size;
            }
        }
        rc = send_data_frame(data_to_send, 1, h2s);
        if (rc != HTTP2_RC_NO_ERROR) {
            return rc;
        }
    }
    // There is not enough window to send it all
    else{
        if(available_data_to_send == 0){
          return HTTP2_RC_NO_ERROR;
        }
        data_to_send = available_data_to_send;
        if (data_to_send > max_frame_size) {
            while (data_to_send > max_frame_size) {
                rc = send_data_frame(max_frame_size, 0, h2s);
                if (rc != HTTP2_RC_NO_ERROR) {
                    return rc;
                }
                data_to_send -= max_frame_size;
            }
        }
        rc = send_data_frame(data_to_send, 0, h2s);
        if (rc != HTTP2_RC_NO_ERROR) {
            return rc;
        }

    }
    decrease_window_available(&h2s->remote_window, available_data_to_send);
    h2s->data.processed += available_data_to_send;
    // All data was processed, data buffer is reset
    if (h2s->data.size == h2s->data.processed) {
        h2s->data.size = 0;
        h2s->data.processed = 0;
        rc = change_stream_state_end_stream_flag(1, h2s); // 1 is for sending
        if (rc == HTTP2_RC_CLOSE_CONNECTION) {
          DEBUG("send_data: Close connection. GOAWAY previously received");
          return rc;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

int send_try_continue_data_sending(h2states_t* h2s)
{
  if(h2s->data.size == 0){
    INFO("try_continue_data_sending: No data left to send.");
    return HTTP2_RC_NO_ERROR;
  }
  int rc = send_data(1, h2s);
  return rc;
}

/*
 * Function: send_local_settings
 * Sends local settings to endpoint.
 * Input: -> st: pointer to hstates_t struct where local settings are stored
 * Output: 0 if settings were sent. -1 if not.
 */
int send_local_settings(h2states_t *h2s)
{
    uint8_t ack = 0; //false
    send_settings_frame(h2s->socket, http2_on_read_continue, ack, h2s->local_settings);
    SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    INFO("Sending settings");
    /*if (rc != size_byte_mysettings) {
        ERROR("Error in local settings writing");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }*/
    /*Settings were sent, so we expect an ack*/
    SET_FLAG(h2s->flag_bits, FLAG_WAIT_SETTINGS_ACK);
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: send_settings_ack
 * Sends an ACK settings frame to endpoint
 * Input: -> st: pointer to hstates struct where http and http2 connection info is
 * stored
 * Output: 0 if sent was successfully made, -1 if not.
 */
int send_settings_ack(h2states_t *h2s)
{
    uint8_t ack = 1; //True
    send_settings_frame(h2s->socket, http2_on_read_continue, ack, NULL);

    SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    INFO("Sending settings ACK");
    /*if (rc != size_byte_ack) {
        ERROR("Error in Settings ACK sending");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }*/
    return HTTP2_RC_NO_ERROR;
}


/*
 * Function: send_ping
 * Sends a ping frame to endpoint
 * Input:
 *      -> opaque_data: opaque data of ping payload
 *      -> ack: boolean if ack != 0 sends an ACK ping, else sends a ping with the ACK flag not set.
 *      -> h2s: pointer to hstates struct where http and http2 connection info is
 * stored
 * Output: HTTP2_RC_NO_ERROR if sent was successfully made, -1 if not.
 */
int send_ping(uint8_t *opaque_data, int8_t ack, h2states_t *h2s) {
    return send_ping_frame(h2s->socket, http2_on_read_continue, opaque_data, ack);
}
/*
 * Function: send_goaway
 * Given an h2states_t struct and a valid error code, sends a GOAWAY FRAME to endpoint.
 * If additional debug data (opaque data) is included it must be written on the
 * given h2states_t.h2s.debug_data_buffer buffer with its corresponding size on the
 * h2states_t.h2s.debug_size variable.
 * IMPORTANT: RFC 7540 section 6.8 indicates that a gracefully shutdown should have
 * an 2^31-1 value on the last_stream_id field and wait at least one RTT before sending
 * a goaway frame with the last stream id. This implementation doesn't.
 * Input: ->st: pointer to h2states_t struct where connection variables are stored
 *        ->error_code: error code for GOAWAY FRAME (RFC 7540 section 7)
 * Output: 0 if no errors were found, -1 if not
 */
int send_goaway(uint32_t error_code, h2states_t *h2s) //, uint8_t *debug_data_buff, uint8_t debug_size){
{
    if (error_code != 0 || FLAG_VALUE(h2s->flag_bits, FLAG_RECEIVED_GOAWAY)) {
        send_goaway_frame(h2s->socket,
                      NULL,
                          error_code,
                          h2s->last_open_stream_id);
    }
    else {
        send_goaway_frame(h2s->socket,
                          http2_on_read_continue,
                          error_code,
                          h2s->last_open_stream_id);
        SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    }
    SET_FLAG(h2s->flag_bits, FLAG_SENT_GOAWAY);
    if (FLAG_VALUE(h2s->flag_bits, FLAG_RECEIVED_GOAWAY)) {
        return HTTP2_RC_CLOSE_CONNECTION;
    }
    return HTTP2_RC_NO_ERROR;
}

int send_rst_stream(uint32_t error_code, h2states_t *h2s)
{
    int rc;
    frame_t frame;
    frame_header_t header;
    rst_stream_payload_t rst_stream_pl;

    create_rst_stream_frame(&header, &rst_stream_pl, h2s->last_open_stream_id, error_code);
    frame.frame_header = &header;
    frame.payload = (void *)&rst_stream_pl;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_bytes, http2_on_read_continue);
    SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    INFO("Sending RST_STREAM, error code: %u", error_code);

    if (rc != bytes_size) {
        ERROR("Error writting RST_STREAM frame. INTERNAL ERROR");
        return HTTP2_RC_ERROR;
    }
    //TODO Set flags and conditions for after sending a RST_STREAM frame
    return HTTP2_RC_NO_ERROR;

}

/*
 * Function: send_window_update
 * Sends connection window update to endpoint.
 * Input: -> st: hstates_t struct pointer where connection variables are stored
 *        -> window_size_increment: increment to put on window_update frame
 * Output: 0 if no errors were found, -1 if not.
 */
int send_window_update(uint8_t window_size_increment, h2states_t *h2s)
{
    //TODO: Refactor this function to avoid duplicated code
    frame_t frame;
    frame_header_t frame_header;
    window_update_payload_t window_update_payload;
    int rc = create_window_update_frame(&frame_header, &window_update_payload, window_size_increment, 0);

    if (rc < 0) {
        ERROR("send_window_update: error creating window_update frame");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }

    frame.frame_header = &frame_header;
    frame.payload = (void *)&window_update_payload;
    uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
    int bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_bytes, http2_on_read_continue);
    SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);

    INFO("Sending connection WINDOW UPDATE");

    if (rc != bytes_size) {
        ERROR("send_window_update: Error writting window_update frame. INTERNAL ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    rc = flow_control_send_window_update(h2s, window_size_increment);
    if (rc != HTTP2_RC_NO_ERROR) {
        ERROR("send_window_update: ERROR in flow control when sending WU - increment too big");
        send_connection_error(HTTP2_PROTOCOL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }

    rc = create_window_update_frame(&frame_header, &window_update_payload, window_size_increment, h2s->current_stream.stream_id);
    if (rc < 0) {
        ERROR("send_window_update: error creating window_update frame");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }

    frame.frame_header = &frame_header;
    frame.payload = (void *)&window_update_payload;
    bytes_size = frame_to_bytes(&frame, buff_bytes);
    rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_bytes, http2_on_read_continue);
    SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);

    INFO("Sending stream WINDOW UPDATE");

    if (rc != bytes_size) {
        ERROR("send_window_update: Error writting window_update frame. INTERNAL ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: send_headers_stream_verification
 * Given an hstates struct, checks the current stream and uses it, creates
 * a new one or reports an error. The stream that will be used is stored in
 * st->h2s.current_stream.stream_id .
 * Input: ->st: hstates_t struct where current stream is stored
 * Output: 0 if no errors were found, -1 if not
 */
int send_headers_stream_verification(h2states_t *h2s)
{
    if (h2s->current_stream.state == STREAM_CLOSED ||
        h2s->current_stream.state == STREAM_HALF_CLOSED_LOCAL) {
        ERROR("Current stream was closed! Send request error. INTERNAL_ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    else if (h2s->current_stream.state == STREAM_IDLE) {
        if (FLAG_VALUE(h2s->flag_bits, FLAG_IS_SERVER)) { // server must use even numbers
            h2s->last_open_stream_id += (h2s->last_open_stream_id % 2) ? 1 : 2;
        }
        else { //stream is closed and id is not zero
            h2s->last_open_stream_id += (h2s->last_open_stream_id % 2) ? 2 : 1;
        }
        h2s->current_stream.state = STREAM_OPEN;
        h2s->current_stream.stream_id = h2s->last_open_stream_id;
    }
    return HTTP2_RC_NO_ERROR;
}


/*
 * Function: send_connection_error
 * Send a connection error to endpoint with a specified error code. It implements
 * the behaviour suggested in RFC 7540, secion 5.4.1
 * Input: -> st: h2states_t pointer where connetion variables are stored
 *        -> error_code: error code that will be used to shutdown the connection
 * Output: void
 */

void send_connection_error(uint32_t error_code, h2states_t *h2s)
{
    int rc = send_goaway(error_code, h2s);

    if (rc < 0) {
        WARN("Error sending GOAWAY frame to endpoint.");
    }
}

void send_stream_error(uint32_t error_code, h2states_t *h2s)
{
    int rc = send_rst_stream(error_code, h2s);

    if (rc < 0) {
        WARN("Error sending RST_STREAM frame to endpoint.");
    }
}

/*
 * Function: send_continuation_frame
 * Sends a single continuation frame to endpoint. It reads the data from the
 * buff_read buffer given as parameter.
 * Input: ->st: hstates_t struct where connection variables are stored.
         ->buff_read: buffer where continuation frame payload is stored
         ->size: number of bytes to read from buff_read and to store in payload
         ->stream_id: stream id to write in continuation payload's header
         ->end_stream: boolean that indicates if END_HEADERS_FLAG must be set
 * Output: 0 if no errors were found during the creation or sending, -1 if not
 */

int send_continuation_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, h2states_t *h2s)
{
    int rc;
    frame_t frame;
    frame_header_t frame_header;
    continuation_payload_t continuation_payload;
    uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];

    create_continuation_frame(buff_read, size, stream_id, &frame_header, &continuation_payload, header_block_fragment);

    if (end_headers) {
        frame_header.flags = set_flag(frame_header.flags, HEADERS_END_HEADERS_FLAG);
    }
    frame.frame_header = &frame_header;
    frame.payload = (void *)&continuation_payload;
    int bytes_size = frame_to_bytes(&frame, buff_read);

    if (end_headers) {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_read, http2_on_read_continue);
        SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    }
    else {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_read, NULL);
    }
    INFO("Sending continuation");
    if (rc != bytes_size) {
        ERROR("Error writting continuation frame. INTERNAL ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: send_headers_frame
 * Send a single headers frame to endpoint. It read the data from the buff_read
 * buffer given as parameter.
 * Input: ->st: hstates_t struct where connection variables are stored
 *        ->buff_read: buffer where headers frame payload is stored
 *        ->size: number of bytes to read from buff_read and to store in payload
 *        ->stream_id: stream id to write on headers frame header
 *        ->end_headers: boolean that indicates if END_HEADERS_FLAG must be set
 *        ->end_stream: boolean that indicates if END_STREAM_FLAG must be set
 * Output: 0 if no errors were found during frame creation/sending, -1 if not
 */

int send_headers_frame(uint8_t *buff_read, int size, uint32_t stream_id, uint8_t end_headers, uint8_t end_stream, h2states_t *h2s)
{
    int rc;
    frame_t frame;
    frame_header_t frame_header;
    headers_payload_t headers_payload;
    uint8_t header_block_fragment[HTTP2_MAX_BUFFER_SIZE];

    // We create the headers frame
    create_headers_frame(buff_read, size, stream_id, &frame_header, &headers_payload, header_block_fragment);

    if (end_headers) {
        frame_header.flags = set_flag(frame_header.flags, HEADERS_END_HEADERS_FLAG);
    }
    if (end_stream) {
        frame_header.flags = set_flag(frame_header.flags, HEADERS_END_STREAM_FLAG);
    }
    frame.frame_header = &frame_header;
    frame.payload = (void *)&headers_payload;
    int bytes_size = frame_to_bytes(&frame, buff_read);
    if (end_headers && end_stream) {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_read, http2_on_read_continue);
        SET_FLAG(h2s->flag_bits, FLAG_WRITE_CALLBACK_IS_SET);
    }
    else {
        rc = event_read_pause_and_write(h2s->socket, bytes_size, buff_read, NULL);
    }
    INFO("Sending headers");
    if (rc != bytes_size) {
        ERROR("Error writting headers frame. INTERNAL ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: send_headers
 * Given an hstates struct, builds and sends a message to endpoint. The message
 * is a sequence of a HEADER FRAME followed by 0 or more CONTINUATION FRAMES.
 * Input: -> st: hstates_t struct where headers are written
        -> end_stream: boolean that indicates if end_stream flag needs to be set
 * Output: 0 if process was made successfully, -1 if not.
 */
int send_headers(uint8_t end_stream, h2states_t *h2s)
{
    if (FLAG_VALUE(h2s->flag_bits, FLAG_RECEIVED_GOAWAY)) {
        ERROR("send_headers: GOAWAY was received. Current process must not open a new stream");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    if (headers_count(&(h2s->headers)) == 0) {
        ERROR("send_headers called when there are no headers to send");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    uint8_t encoded_bytes[HTTP2_MAX_BUFFER_SIZE];
    int size = compress_headers(&h2s->headers, encoded_bytes, &h2s->hpack_states);
    if (size < 0) {
        ERROR("Error was found compressing headers. INTERNAL ERROR");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    if (send_headers_stream_verification(h2s) < 0) {
        ERROR("Stream error during the headers sending. INTERNAL ERROR"); // error already handled
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    uint32_t stream_id = h2s->current_stream.stream_id;
    uint32_t max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
    int rc;
    //not being considered dependencies nor padding.
    if ((uint32_t)size <= max_frame_size) { //if headers can be send in only one frame
        //only send 1 header
        rc = send_headers_frame(encoded_bytes, size, stream_id, 1, end_stream, h2s);
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error found sending headers frame");
            return rc;
        }
        if (end_stream) {
            rc = change_stream_state_end_stream_flag(1, h2s); // 1 is for sending
            if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                DEBUG("handle_headers_payload: Close connection. GOAWAY previously received");
                return rc;
            }
        }
        return rc;  // should be HTTP2_RC_NO_ERROR
    }
    else {          //if headers must be send with one or more continuation frames
        uint32_t remaining = size;
        //send Header Frame
        rc = send_headers_frame(encoded_bytes, max_frame_size, stream_id, 0, end_stream, h2s);
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error found sending headers frame");
            return rc;
        }
        remaining -= max_frame_size;
        //Send continuation Frames
        while (remaining > max_frame_size) {
            rc = send_continuation_frame(encoded_bytes + (size - remaining), max_frame_size, stream_id, 0, h2s);
            if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
                ERROR("Error found sending continuation frame");
                return rc;
            }
            remaining -= max_frame_size;
        }
        //send last continuation frame
        rc = send_continuation_frame(encoded_bytes + (size - remaining), remaining, stream_id, 1, h2s);
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error found sending continuation frame");
            return rc;
        }
        if (end_stream) {
            rc = change_stream_state_end_stream_flag(1, h2s); // 1 is for sending
            if (rc == HTTP2_RC_CLOSE_CONNECTION) {
                DEBUG("handle_headers_payload: Close connection. GOAWAY previously received");
                return rc;
            }
        }
        return rc; // should be HTTP2_RC_NO_ERROR
    }
}

/*
 * Function: send_response
 * Given a set of headers and, in some cases, data, generates an http2 message
 * to be send to endpoint. The message is a response, so it must be sent through
 * an existent stream, closing the current stream.
 *
 * @param    h2s       pointer to hstates_t struct where headers are stored
 *
 * @return   0         Successfully generation and sent of the message
 * @return   -1        There was an error in the process
 */
int send_response(h2states_t *h2s)
{
    if (headers_count(&(h2s->headers)) == 0) {
        ERROR("There were no headers to write");
        send_connection_error(HTTP2_INTERNAL_ERROR, h2s);
        return HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT;
    }
    int rc;
    if (h2s->data.size > 0) {
        rc = send_headers(0, h2s); // CLOSE CONN, CLOSE CONN SENT, NO ERROR
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error was found sending headers on response");
            return rc;
        }
        else if (rc == HTTP2_RC_CLOSE_CONNECTION) {
            DEBUG("send_response: Close connection. GOAWAY sent while on send_headers.");
            return rc;
        }
        rc = send_data(1, h2s); // CLOSE CONN, CLOSE CONN SENT, NO ERROR
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error was found sending data on response");
            return rc;
        }
    }
    else {
        rc = send_headers(1, h2s);
        if (rc == HTTP2_RC_CLOSE_CONNECTION_ERROR_SENT) {
            ERROR("Error was found sending headers on response");
            return rc;
        }
        else if (rc == HTTP2_RC_CLOSE_CONNECTION) {
            DEBUG("send_response: Close connection. GOAWAY sent while on send_headers.");
            return rc;
        }
    }
    return HTTP2_RC_NO_ERROR;
}
