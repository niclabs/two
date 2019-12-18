#ifndef HTTP2_FLOWCONTROL_H
#define HTTP2_FLOWCONTROL_H

#include "http2/structs.h"

uint32_t update_window_size(h2states_t* h2s, uint32_t initial_window_size, uint8_t place);
int32_t get_window_available_size(h2_flow_control_window_t flow_control_window);
int decrease_window_available(h2_flow_control_window_t* flow_control_window, uint32_t data_size);
int increase_window_available(h2_flow_control_window_t* flow_control_window, uint32_t window_size_increment);

int flow_control_receive_data(h2states_t* st, uint32_t length);
int flow_control_send_window_update(h2states_t* st, uint32_t window_size_increment);
int flow_control_receive_window_update(h2states_t* st, uint32_t window_size_increment);

uint32_t get_sending_available_window(h2states_t *st);


#endif /*HTTP2_FLOWCONTROL_H*/
