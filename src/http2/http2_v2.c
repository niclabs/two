#include "http2_v2.h"
#include "logging.h"
#include <string.h>
#include "http2/check.h"
#include "http2_send.h"
#include "http2_utils_v2.h"
#include "http2_flowcontrol_v2.h"
#include "http.h"


callback_t h2_server_init_connection(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_header(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload_wait_settings_ack(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
callback_t receive_payload_goaway(cbuf_t *buf_in, cbuf_t *buf_out, void *state);
int check_incoming_condition(cbuf_t *buf_out, h2states_t *h2s);
/*
int send_goaway(uint32_t error_code, cbuf_t *buf_out, h2states_t *h2s);
void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s);
*/
int handle_payload(uint8_t *buff_read, cbuf_t *buf_out, h2states_t *h2s);
int handle_data_payload(frame_header_t *frame_header, data_payload_t *data_payload, cbuf_t *buf_out, h2states_t* h2s);
int handle_headers_payload(frame_header_t *header, headers_payload_t *hpl, cbuf_t *buf_out, h2states_t *h2s);
int handle_settings_payload(settings_payload_t *spl, cbuf_t *buf_out, h2states_t *h2s);
int handle_goaway_payload(goaway_payload_t *goaway_pl, cbuf_t *buf_out, h2states_t *h2s);
int handle_continuation_payload(frame_header_t *header, continuation_payload_t *contpl, cbuf_t *buf_out, h2states_t *h2s);
int handle_window_update_payload(cbuf_t *buf_out, h2states_t *h2s);

int init_variables_h2s(h2states_t *h2s, uint8_t is_server)
{
    h2s->is_server = is_server;
    h2s->remote_settings[0] = h2s->local_settings[0] = DEFAULT_HEADER_TABLE_SIZE;
    h2s->remote_settings[1] = h2s->local_settings[1] = DEFAULT_ENABLE_PUSH;
    h2s->remote_settings[2] = h2s->local_settings[2] = DEFAULT_MAX_CONCURRENT_STREAMS;
    h2s->remote_settings[3] = h2s->local_settings[3] = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->remote_settings[4] = h2s->local_settings[4] = DEFAULT_MAX_FRAME_SIZE;
    h2s->remote_settings[5] = h2s->local_settings[5] = DEFAULT_MAX_HEADER_LIST_SIZE;
    h2s->wait_setting_ack = 0;
    h2s->current_stream.stream_id = is_server ? 2 : 3;
    h2s->current_stream.state = STREAM_IDLE;
    h2s->last_open_stream_id = 1;
    h2s->header_block_fragments_pointer = 0;
    h2s->waiting_for_end_headers_flag = 0;
    h2s->received_end_stream = 0;
    h2s->incoming_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->incoming_window.window_used = 0;
    h2s->outgoing_window.window_size = DEFAULT_INITIAL_WINDOW_SIZE;
    h2s->outgoing_window.window_used = 0;
    h2s->sent_goaway = 0;
    h2s->received_goaway = 0;
    h2s->debug_size = 0;
    //h2s->header = NULL;
    hpack_init_states(&(h2s->hpack_states), DEFAULT_HEADER_TABLE_SIZE);

    h2s->headers.headers = h2s->headers_buf;
    h2s->headers.count = 0;
    h2s->headers.maxlen = HTTP2_MAX_HEADER_COUNT;
    return 0;
}

callback_t h2_server_init_connection(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{
    if (cbuf_len(buf_in) < 24) {
        callback_t ret = { h2_server_init_connection, NULL };
        return ret;
    }
    int rc;
    uint8_t preface_buff[25];
    char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    rc = cbuf_pop(buf_in, preface_buff, 24);
    if (rc != 24) {
        DEBUG("Error during cbuf_pop operation");
        return null_callback();
    }
    preface_buff[24] = '\0';
    h2states_t *h2s = (h2states_t *)state;
    if (strcmp(preface, (char *)preface_buff) != 0) {
        ERROR("Error in preface received");
        send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
        return null_callback();
    }
    // send connection settings
    rc = init_variables_h2s(h2s, 1);
    callback_t ret_null = { NULL, NULL };
    return ret_null;
}

callback_t receive_header(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{
    if (cbuf_len(buf_in) < 9) {
        callback_t ret = { receive_header, NULL };
        return ret;
    }
    h2states_t *h2s = (h2states_t *)state;
    frame_header_t header;
    uint8_t buff_read_header[10];
    int rc = cbuf_pop(buf_in, buff_read_header, 9);

    if (rc != 9) {
        WARN("READ %d BYTES FROM SOCKET", rc);
        return null_callback();
    }
    rc = bytes_to_frame_header(buff_read_header, 9, &header);
    if (rc) {
        ERROR("Error coding bytes to frame header. INTERNAL_ERROR");
        send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
        return null_callback();
    }
    if (header.length > read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE)) {
        ERROR("Length of the frame payload greater than expected. FRAME_SIZE_ERROR");
        send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
        return null_callback();
    }
    // save header type
    h2s->header = header;
    // If errors are found, internal logic will handle them.
    rc = check_incoming_condition(buf_out, h2s);
    if(rc < 0){
      DEBUG("Error was found during incoming condition checking");
      return null_callback();
    }
    else if(rc == 1){
      callback_t ret = {receive_header, NULL};
      return ret;
    }
    callback_t ret = { receive_payload, NULL };
    return ret;
}

callback_t receive_payload(cbuf_t *buf_in, cbuf_t *buf_out, void *state)
{   (void) buf_out;
    h2states_t *h2s = (h2states_t *)state;

    if (cbuf_len(buf_in) < h2s->header.length) {
        callback_t ret = { receive_payload, NULL };
        return ret;
    }

    uint8_t buff_read_payload[HTTP2_MAX_BUFFER_SIZE];
    int rc = cbuf_pop(buf_in, buff_read_payload, h2s->header.length);
    if (rc != h2s->header.length) {
        ERROR("Error reading bytes of payload, read %d bytes", rc);
        return null_callback();
    }
    rc = handle_payload(buff_read_payload, buf_out, h2s);
    // todo: read_type_payload from buff_read_payload

    // placeholder
    callback_t ret_null = { NULL, NULL };
    return ret_null;
}



int check_incoming_condition(cbuf_t *buf_out, h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            rc = check_incoming_data_condition(buf_out, h2s);
            return rc;
        }
        case HEADERS_TYPE: {
            rc = check_incoming_headers_condition(buf_out, h2s);
            return rc;

        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;

        case SETTINGS_TYPE: {
            rc = check_incoming_settings_condition(buf_out, h2s);
            return rc;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;

        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;

        case GOAWAY_TYPE: {
            rc = check_incoming_goaway_condition(buf_out, h2s);
            return rc;
        }
        case WINDOW_UPDATE_TYPE: {
            return 0;
        }
        case CONTINUATION_TYPE: {
            rc = check_incoming_continuation_condition(buf_out, h2s);
            return rc;
        }
        default: {
            WARN("Error: Type not found");
            return -1;

        }
    }
}

/*
*
*
*
*/
int handle_payload(uint8_t *buff_read, cbuf_t *buf_out, h2states_t *h2s)
{
    int rc;

    switch (h2s->header.type) {
        case DATA_TYPE: {
            DEBUG("handle_payload: RECEIVED DATA PAYLOAD");
            data_payload_t data_payload;
            //uint8_t data[h2s->header.length]; CHECK WITH HPACK PEOPLE
            rc = h2s->header.callback(&(h2s->header), &data_payload, buff_read);
            if(rc < 0){
                ERROR("ERROR reading data payload");
                return -1;
            }
            rc = handle_data_payload(&(h2s->header), &data_payload, buf_out, h2s);
            if(rc < 0){
                ERROR("ERROR in handle receive data");
                return -1;
            }
            DEBUG("handle_payload: RECEIVED DATA PAYLOAD OK");
            return 0;
        }
        case HEADERS_TYPE: {
            DEBUG("handle_payload: RECEIVED HEADERS PAYLOAD");
            
            headers_payload_t hpl;
            uint8_t headers_block_fragment[HTTP2_MAX_HBF_BUFFER];
            uint8_t padding[32];
            hpl.header_block_fragment=headers_block_fragment;
            hpl.padding = padding;

            rc = h2s->header.callback(&(h2s->header), &hpl, buff_read);
            if(rc < 0){
                ERROR("ERROR reading headers payload");
                return rc;
            }
            rc = handle_headers_payload(&(h2s->header), &hpl, buf_out, h2s);
            if(rc < 0){
                ERROR("ERROR in handle receive data");
                return -1;
            }
            DEBUG("handle_payload: RECEIVED HEADERS PAYLOAD OK");
            return 0;

        }
        case PRIORITY_TYPE://Priority
            WARN("TODO: Priority Frame. Not implemented yet.");
            return -1;

        case RST_STREAM_TYPE://RST_STREAM
            WARN("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;

        case SETTINGS_TYPE: {
            DEBUG("handle_payload: RECEIVED SETTINGS PAYLOAD");
            
            settings_payload_t spl;
            settings_pair_t pairs[h2s->header.length/6];
            spl.pairs = pairs;

            rc = h2s->header.callback(&(h2s->header), &spl, buff_read);
            if(rc < 0){
              // bytes_to_settings_payload returns -1 if length is not a multiple of 6. RFC 6.5
              send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
              return rc;
            }
            rc = handle_settings_payload(&spl, buf_out, h2s);
            if(rc < 0){
                ERROR("ERROR in handle settings payload!");
                return -1;
            }
            DEBUG("handle_payload: RECEIVED HEADERS PAYLOAD OK");
            return 0;
        }
        case PUSH_PROMISE_TYPE://Push promise
            WARN("TODO: Push promise frame. Not implemented yet.");
            return -1;

        case PING_TYPE://Ping
            WARN("TODO: Ping frame. Not implemented yet.");
            return -1;

        case GOAWAY_TYPE: {
            DEBUG("handle_payload: RECEIVED GOAWAY PAYLOAD");
            
            uint16_t max_frame_size = read_setting_from(h2s, LOCAL, MAX_FRAME_SIZE);
            uint8_t debug_data[max_frame_size - 8];
            goaway_payload_t goaway_pl;
            goaway_pl.additional_debug_data = debug_data;

            rc = h2s->header.callback(&(h2s->header), &goaway_pl, buff_read);
            if(rc < 0){
              ERROR("Error in reading goaway payload");
              return -1;
            }
            rc = handle_goaway_payload(&goaway_pl, buf_out, h2s);
            if(rc < 0){
              ERROR("Error during goaway handling");
              return -1;
            }
            DEBUG("handle_payload: RECEIVED GOAWAY PAYLOAD OK");
            return 0;
        }
        case WINDOW_UPDATE_TYPE: {
            DEBUG("handle_payload: RECEIVED WINDOW_UPDATE PAYLOAD");
            window_update_payload_t window_update_payload;
            int rc = h2s->header.callback(&(h2s->header), &window_update_payload, buff_read);
            if (rc < 0) {
                ERROR("Error in reading window_update_payload. FRAME_SIZE_ERROR");
                send_connection_error(buf_out, HTTP2_FRAME_SIZE_ERROR, h2s);
                return -1;
            }
            uint32_t window_size_increment = window_update_payload.window_size_increment;
            if(window_size_increment == 0){
              ERROR("Flow-control window increment is 0. Stream Error. PROTOCOL_ERROR");
              send_connection_error(buf_out, HTTP2_PROTOCOL_ERROR, h2s);
              return -1;
            }
            rc = flow_control_receive_window_update(h2s, window_size_increment);
                if(rc < 0){
                    send_connection_error(buf_out, HTTP2_FLOW_CONTROL_ERROR, h2s);
                    return -1;
                }
            DEBUG("handle_payload: RECEIVED WINDOW_UPDATE PAYLOAD OK");
            return 0;
        }
        case CONTINUATION_TYPE: {
            DEBUG("handle_payload: RECEIVED CONTINUATION PAYLOAD");
            
            continuation_payload_t contpl;
            uint8_t continuation_block_fragment[HTTP2_MAX_HBF_BUFFER - h2s->header_block_fragments_pointer];
            contpl.header_block_fragment = continuation_block_fragment;

            rc = h2s->header.callback(&(h2s->header), &contpl, buff_read);
            if(rc < 0){
              ERROR("Error in continuation payload reading");
              send_connection_error(buf_out, HTTP2_INTERNAL_ERROR, h2s);
              return -1;
            }
            rc = handle_continuation_payload(&h2s->header, &contpl, buf_out, h2s);
            if(rc < 0){
              ERROR("Error was found during continuatin payload handling");
              return -1;
            }
            DEBUG("handle_payload: RECEIVED CONTINUATION PAYLOAD OK");
            return 0;
        }
        default: {
            WARN("Error: Type not found");
            return -1;
        }
    }
}
