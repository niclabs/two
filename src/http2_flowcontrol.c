#include "http2_flowcontrol.h"
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
* Input: ->window_manager: h2h2_window_manager_t struct where window info is stored
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

int flow_control_receive_data(hstates_t* st, uint32_t length){
    uint32_t window_available = get_window_available_size(st->h2s.incoming_window);
    if(length > window_available){
        ERROR("FLOW_CONTROL_ERROR found");
        return -1;
    }
    increase_window_used(&st->h2s.incoming_window, length);
    return 0;
}

int flow_control_send_window_update(hstates_t* st, uint32_t window_size_increment){
    if(window_size_increment>st->h2s.incoming_window.window_used){
        ERROR("Increment to big. protocol_error");
        return -1;
    }
    decrease_window_used(&st->h2s.incoming_window,window_size_increment);
    return 0;
}

int flow_control_receive_window_update(hstates_t* st, uint32_t window_size_increment){
    if(window_size_increment>st->h2s.outgoing_window.window_used){
        ERROR("Increment to big. protocol_error");
        return -1;
    }
    decrease_window_used(&st->h2s.outgoing_window,window_size_increment);
    return 0;
}