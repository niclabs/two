#include "http2_flowcontrol_v2.h"
#include "logging.h"

/*
* Function get_window_available_size
* Returns the available window size of the given window manager. It indicates
* the octets available in the corresponding endpoint to process data.
* Input: ->window_manager: h2_window_manager_t struct where window info is stored
* Output: The available window size
*/
uint32_t get_window_available_size(h2_window_manager_t window_manager){
    return window_manager.window_size - window_manager.window_used;
}

/*
* Function: increase_window_used
* Increases the window_used value on a given window manager.
* Input: ->window_manager: h2_window_manager_t struct pointer where window info is stored
*        ->data_size: the corresponding increment on the window used
* Output: 0
*/
int increase_window_used(h2_window_manager_t* window_manager, uint32_t data_size){
    window_manager->window_used += data_size;
    return 0;
}

/*
* Function: decrease_window_used
* Decreases the window_used value on a given window manager.
* Input: ->window_manager: h2h2_window_manager_t struct where window info is stored
*        ->data_size: the corresponding decrement on the window used
* Output: 0
*/
int decrease_window_used(h2_window_manager_t* window_manager, uint32_t window_size_increment){
    window_manager->window_used -= window_size_increment;
    return 0;
}

/*
* Function: flow_control_receive_data
* Checks if the incoming data fits on the available window.
* Input: -> st: hstates_t struct pointer where connection windows are stored
*        -> length: the size of the incoming data received
* Output: 0 if data can be processed. -1 if not
*/
int flow_control_receive_data(h2states_t *h2s, uint32_t length){
    uint32_t window_available = get_window_available_size(h2s->incoming_window);
    if(length > window_available){
        ERROR("Available window is smaller than data received. FLOW_CONTROL_ERROR");
        return -1;
    }
    increase_window_used(&h2s->incoming_window, length);
    return 0;
}

/*
* Function: flow_control_send_data
* Updates the outgoing window when certain amount of data is sent.
* Input: -> st: hstates_t struct pointer where connection windows are stored
*        -> data_sent: the size of the outgoing data.
* Output: 0 if window increment was successfull. -1 if not
*/
int flow_control_send_data(h2states_t *h2s, uint32_t data_sent){
  if(data_sent > get_window_available_size(h2s->outgoing_window)){
    ERROR("Trying to send more data than allowed by window.");
    return -1;
  }
  increase_window_used(&h2s->outgoing_window, data_sent);
  return 0;
}

int flow_control_send_window_update(h2states_t *h2s, uint32_t window_size_increment){
    if(window_size_increment>h2s->incoming_window.window_used){
        ERROR("Increment to big. protocol_error");
        return -1;
    }
    decrease_window_used(&h2s->incoming_window,window_size_increment);
    return 0;
}

int flow_control_receive_window_update(h2states_t *h2s, uint32_t window_size_increment){
    if(window_size_increment>h2s->outgoing_window.window_used){
        ERROR("Flow control: window increment bigger than window used. PROTOCOL_ERROR");
        return -1;
    }
    decrease_window_used(&h2s->outgoing_window,window_size_increment);
    return 0;
}


uint32_t get_size_data_to_send(h2states_t *h2s){
    uint32_t available_window = get_window_available_size(h2s->outgoing_window);
    if( available_window <= h2s->data.size - h2s->data.processed){
        return available_window;
    }
    else{
        return h2s->data.size - h2s->data.processed;
    }
    return 0;
}
