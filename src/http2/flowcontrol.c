#include "http2/flowcontrol.h"

// Specify to which module this file belongs
#include "config.h"
#define LOG_MODULE LOG_MODULE_HTTP2
#include "logging.h"

/*
 * Function get_window_available_size
 * Returns the available window size of the given window manager. It indicates
 * the octets available in the corresponding endpoint to process data.
 * Input: ->window_manager: h2_window_manager_t struct where window info is stored
 * Output: The available window size
 */
int get_window_available_size(h2_flow_control_window_t flow_control_window)
{
    return flow_control_window.stream_window;
}

/*
 * Function update_window_size
 * Update window size value and window used value if necessary.
 * @param     window_manager     h2_window_manager_t struct where window info is stored
 * @param     new_window_size    new value of window size
 *
 * @returns   1
 */
uint32_t update_window_size(h2states_t *h2s, uint32_t initial_window_size, uint8_t place)
{
    int id = INITIAL_WINDOW_SIZE;

    if (place == LOCAL) {
        h2s->local_window.stream_window += initial_window_size - h2s->local_settings[--id];
        // An endpoint MUST treat a change to SETTINGS_INITIAL_WINDOW_SIZE that
        // causes any flow-control window to exceed the maximum size as a
        // connection error of type FLOW_CONTROL_ERROR.
        if (h2s->local_window.stream_window > 2147483647) {
            return HTTP2_RC_ERROR;
        }
    }
    else if (place == REMOTE) {
        h2s->remote_window.stream_window += initial_window_size - h2s->remote_settings[--id];
        if (h2s->remote_window.stream_window > 2147483647) {
            return HTTP2_RC_ERROR;
        }
    }
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: increase_window_used
 * Increases the window_used value on a given window manager.
 * Input: ->flow_control_window: h2_flow_control_window_t struct pointer where window info is stored
 *        ->data_size: the corresponding increment on the window used
 * Output: 0
 */
int decrease_window_available(h2_flow_control_window_t *flow_control_window, uint32_t data_size)
{
    flow_control_window->stream_window -= data_size;
    flow_control_window->connection_window -= data_size;
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: flow_control_receive_data
 * Checks if the incoming data fits on the available window.
 * Input: -> st: hstates_t struct pointer where connection windows are stored
 *        -> length: the size of the incoming data received
 * Output: 0 if data can be processed. -1 if not
 */
int flow_control_receive_data(h2states_t *h2s, uint32_t length)
{
    int window_available = get_window_available_size(h2s->local_window);

    if (window_available < 0) {
        DEBUG("Local available window is negative. No data can be processed");
        return HTTP2_RC_ERROR;
    }

    if (length > (uint32_t)window_available) {
        ERROR("Available window is smaller than data received. FLOW_CONTROL_ERROR");
        return HTTP2_RC_ERROR;
    }
    decrease_window_available(&h2s->local_window, length);
    return HTTP2_RC_NO_ERROR;
}

/*
 * Function: flow_control_send_data
 * Updates the outgoing window when certain amount of data is sent.
 * Input: -> st: hstates_t struct pointer where connection windows are stored
 *        -> data_sent: the size of the outgoing data.
 * Output: 0 if window increment was successfull. -1 if not
 */
int flow_control_send_data(h2states_t *h2s, uint32_t data_sent)
{
    if ((int)data_sent > get_window_available_size(h2s->remote_window)) {
        ERROR("Trying to send more data than allowed by window.");
        return HTTP2_RC_ERROR;
    }
    decrease_window_available(&h2s->remote_window, data_sent);
    return HTTP2_RC_NO_ERROR;
}

int flow_control_send_window_update(h2states_t *h2s, uint32_t window_size_increment)
{
    h2s->local_window.stream_window += window_size_increment;
    h2s->local_window.connection_window += window_size_increment;
    if (h2s->local_window.stream_window > 2147483647 || h2s->local_window.connection_window > 2147483647) {
        ERROR("Increment to big. PROTOCOL_ERROR");
        return HTTP2_RC_ERROR;
    }

    return HTTP2_RC_NO_ERROR;
}

int flow_control_receive_window_update(h2states_t *h2s, uint32_t window_size_increment)
{
    if (h2s->header.stream_id == 0) {
        h2s->remote_window.connection_window += window_size_increment;
        if (h2s->remote_window.connection_window < 0) {
            DEBUG("flow_control_receive_window_update: Remote connection window overflow!");
            return HTTP2_RC_ERROR;
        }
    }
    else if (h2s->header.stream_id == h2s->current_stream.stream_id) {
        h2s->remote_window.stream_window += window_size_increment;
        if (h2s->remote_window.stream_window < 0) {
            DEBUG("flow_control_receive_window_update: Remote stream window overflow!");
            return HTTP2_RC_ERROR;
        }
    }
    return HTTP2_RC_NO_ERROR;
}


uint32_t get_sending_available_window(h2states_t *h2s)
{
    int available_window = get_window_available_size(h2s->remote_window);

    // Window is 0 or negative
    if (available_window < 1) {
        return 0;
    }
    return (uint32_t)available_window;

}
