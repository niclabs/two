#ifndef HTTP2_FLOWCONTROL_H
#define HTTP2_FLOWCONTROL_H
#include "http_bridge.h"

uint32_t get_window_available_size(h2_window_manager_t window_manager);
int increase_window_used(h2_window_manager_t* window_manager, uint32_t data_size);
int decrease_window_used(h2_window_manager_t* window_manager, uint32_t window_size_increment);

int flow_control_receive_data(hstates_t* st, uint32_t length);
int flow_control_send_window_update(hstates_t* st, uint32_t window_size_increment);
int flow_control_receive_window_update(hstates_t* st, uint32_t window_size_increment);


#endif /*HTTP2_FLOWCONTROL_H*/
