#include "http2_send.h"
#include "frames.h"
#include "headers.h"
#include "logging.h"

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
int send_goaway(uint32_t error_code, cbuf_t *buf_out, h2states_t *h2s){//, uint8_t *debug_data_buff, uint8_t debug_size){
  int rc;
  frame_t frame;
  frame_header_t header;
  goaway_payload_t goaway_pl;
  uint8_t additional_debug_data[h2s->debug_size];
  rc = create_goaway_frame(&header, &goaway_pl, additional_debug_data, h2s->last_open_stream_id, error_code, h2s->debug_data_buffer, h2s->debug_size);
  if(rc < 0){
    ERROR("Error creating GOAWAY frame");
    //TODO shutdown connection
    return -1;
  }
  frame.frame_header = &header;
  frame.payload = (void*)&goaway_pl;
  uint8_t buff_bytes[HTTP2_MAX_BUFFER_SIZE];
  int bytes_size = frame_to_bytes(&frame, buff_bytes);
  rc = cbuf_push(buf_out, buff_bytes, bytes_size);
  INFO("Sending GOAWAY, error code: %u", error_code);

  if(rc != bytes_size){
    ERROR("Error writting goaway frame. INTERNAL ERROR");
    //TODO shutdown connection
    return -1;
  }
  h2s->sent_goaway = 1;
  if(h2s->received_goaway){
    return -1;
  }
  return 0;

}

/*
* Function: send_connection_error
* Send a connection error to endpoint with a specified error code. It implements
* the behaviour suggested in RFC 7540, secion 5.4.1
* Input: -> st: h2states_t pointer where connetion variables are stored
*        -> error_code: error code that will be used to shutdown the connection
* Output: void
*/

void send_connection_error(cbuf_t *buf_out, uint32_t error_code, h2states_t *h2s){
  int rc = send_goaway(error_code, buf_out, h2s);
  if(rc < 0){
    WARN("Error sending GOAWAY frame to endpoint.");
  }
}
