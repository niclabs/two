#include "http2utils.h"

/*
* This API assumes the existence of a TCP connection between Server and Client,
* and two methods to use this TCP connection that are tcp_write and tcp_write
*/

/*--- Global Variables ---*/
static uint32_t remote_settings[6];
static uint32_t local_settings[6];
//static uint32_t local_cache[6];
//static uint8_t waiting_sett_ack = 0;

/*--------------Internal methods-----------------------*/

/*
* Function: init_settings_tables
* Initialize default values of remote and local settings
* Input: void
* Output: 0 if initialize were made. -1 if not.
*/
int init_settings_tables(void){
  remote_settings[0] = local_settings[0] = DEFAULT_HTS;
  remote_settings[1] = local_settings[1] = DEFAULT_EP;
  remote_settings[2] = local_settings[2] = DEFAULT_MCS;
  remote_settings[3] = local_settings[3] = DEFAULT_IWS;
  remote_settings[4] = local_settings[4] = DEFAULT_MFS;
  remote_settings[5] = local_settings[5] = DEFAULT_MHLS;
  return 0;
}

/*
* Function: update_remote_settings
* Updates the table where remote settings are stored
* Input: -> sframe, it must be a SETTINGS frame
*        -> place, must be LOCAL or REMOTE. It indicates which table to update.
* Output: 0 if update was successfull, -1 if not
*/
int update_settings_table(settingspayload_t *spl, uint8_t place){
  /*spl is for setttings payload*/
  uint8_t i;
  uint16_t id;
  int rc = 0;
  /*Verify the values of settings*/
  for(i = 0; i < spl->count; i++){
    rc+=verify_setting(spl->pairs[i].identifier, spl->pairs[i].value);
  }
  if(rc != 0){
    puts("Error: invalid setting found");
    return -1;
  }
  if(place == REMOTE){
    /*Update remote table*/
    for(i = 0; i < spl->count; i++){
      id = spl->pairs[i].identifier;
      remote_settings[--id] = spl->pairs[i].value;
    }
    return 0;
  }
  else if(place == LOCAL){
    /*Update local table*/
    for(i = 0; i < spl->count; i++){
      id = spl->pairs[i].identifier;
      local_settings[--id] = spl->pairs[i].value;
    }
    return 0;
  }
  else{
    puts("Error: Not a valid table to update");
    return -1;
  }
}

/*
* Function: send_settings_ack
* Sends an ACK settings frame to endpoint
* Input: void
* Output: 0 if sent was successfully made, -1 if not.
*/
int send_settings_ack(void){
  frame_t ack_frame;
  frameheader_t ack_frame_header;
  int rc;
  createSettingsAckFrame(&ack_frame, &ack_frame_header);
  uint8_t byte_ack[9+0]; /*Settings ACK frame only has a header*/
  int size_byte_ack = frameToBytes(&ack_frame, byte_ack);
  /*TODO: tcp_write*/
  rc = tcp_write(byte_ack, size_byte_ack);
  if(rc != size_byte_ack){
    puts("Error in Settings ACK sending");
    return -1;
  }
  return 0;
}

/*
* Function: read_settings_payload
* Given a buffer and a pointer to a header, operates a settings frame payload.
* Input: -> buff_read: buffer where payload's data is written
        -> header: pointer to a frameheader_t structure already built
* Output: 0 if operations are done successfully, -1 if not.
*/
int read_settings_payload(uint8_t *buff_read, frameheader_t *header){
  settingspayload_t settings_payload;
  settingspair_t pairs[header->length/6];
  int size = bytesToSettingsPayload(buff_read,header->length, &settings_payload, pairs);
  if(size != header->length){
    puts("Error in byte to settings payload coding");
    return -1;
  }
  if(!update_settings_table(&settings_payload, REMOTE)){
    send_settings_ack();
    return 0;
  }
  else{
    /*TODO: send protocol error*/
    return -1;
  }
}

/*
* Function: read_frame
* Build a header and writes the header's payload's bytes on buffer.
* Input: -> buff_read: buffer where payload bytes are going to be stored.
        -> header: pointer to the frameheader_t where header is going to be stored
* Output: 0 if writing and building is done successfully. -1 if not.
*/
int read_frame(uint8_t *buff_read, frameheader_t *header){
  int rc = read_n_bytes(buff_read, 9);
  if(rc != 9){
    puts("Error reading bytes from http");
    return -1;
  }
  rc = bytesToFrameHeader(buff_read, 9, header);
  if(rc != 9){
    puts("Error coding bytes to frame header");
    return -1;
  }
  if(header->length > 256){
    puts("Error: Payload's size too big (>256)");
    return -1;
  }
  rc = read_n_bytes(buff_read, header->length);
  if(rc != header->length){
    puts("Error reading bytes from http");
    return -1;
  }
  return 0;
}

/*----------------------API methods-------------------*/

/*
* Function: send_local_settings
* Sends local settings to endpoint
* Input: void
* Output: 0 if settings were sent. -1 if not.
*/
int send_local_settings(void){
  int rc;
  uint16_t ids[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
  frame_t mysettingframe;
  frameheader_t mysettingframeheader;
  settingspayload_t mysettings;
  settingspair_t mypairs[6];
  /*rc must be 0*/
  rc = createSettingsFrame(ids, local_settings, 6, &mysettingframe,
                            &mysettingframeheader, &mysettings, mypairs);
  if(rc){
    puts("Error in Settings Frame creation");
    return -1;
    }
  uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
  int size_byte_mysettings = frameToBytes(&mysettingframe, byte_mysettings);
  /*Assuming that tcp_write returns the number of bytes written*/
  rc = tcp_write(byte_mysettings, size_byte_mysettings);
  if(rc != size_byte_mysettings){
    puts("Error in local settings writing");
    return -1;
  }
  return 0;
}

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. 0 if nothing was read.
*/

uint32_t read_setting_from(uint8_t place, uint8_t param){
  if(param < 1 || param > 6){
    printf("Error: %u is not a valid setting parameter", param);
    return 0;
  }
  if(place == LOCAL){
    return local_settings[--param];
  }
  else if(place == REMOTE){
    return remote_settings[--param];
  }
  else{
    puts("Error: not a valid table to read from");
    return 0;
  }
}

/*
* Function: client_init_connection
* Initializes HTTP2 connection between endpoints. Sends preface and local
* settings.
* Input: void
* Output: 0 if connection was made successfully. -1 if not.
*/
int client_init_connection(void){
  int rc = init_settings_tables();
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  uint8_t preface_buff[24];
  puts("Client: sending preface...");
  uint8_t i = 0;
  /*We load the buffer with the ascii characters*/
  while(preface[i] != '\0'){
    preface_buff[i] = preface[i];
    i++;
  }
  rc = tcp_write(preface_buff,24);
  if(rc != 24){
    puts("Error in preface sending");
    return -1;
  }
  puts("Client: sending local settings...");
  if((rc = send_local_settings()) < 0){
    puts("Error in local settings sending");
    return -1;
  }
  puts("Client: init connection done");
  return 0;
}

/*
* Function: server_init_connection
* Initializes HTTP2 connection between endpoints. Waits for preface and sends
* local settings.
* Input: void
* Output: 0 if connection was made successfully. -1 if not.
*/
int server_init_connection(void){
  int rc = init_settings_tables();
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  uint8_t preface_buff[25];
  preface_buff[24] = '\0';
  puts("Server: waiting for preface...");
  /*We read the first 24 byes*/
  rc = read_n_bytes(preface_buff, 24);
  puts("Server: 24 bytes read");
  if(rc != 24){
    puts("Error in reading preface");
    return -1;
  }
  if(strcmp(preface, (char*)preface_buff) != 0){
    puts("Error in preface receiving");
    return -1;
  }
  /*Server sends local settings to endpoint*/
  puts("Server: sending local settings");
  if((rc = send_local_settings()) < 0){
    puts("Error in local settings sending");
    return -1;
  }
  puts("Server: init connection done");
  return 0;
}

/*
* Function: receive_frame
* Receives a frame from endpoint, decodes it and works with it.
* Input: void
* Output: 0 if no problem was found. -1 if error was found.
*/
int receive_frame(void){
  uint8_t buff_read[MAX_BUFFER_SIZE];
  //uint8_t buff_write[MAX_BUFFER_SIZE]
  int rc;
  frameheader_t header;
  rc = read_frame(buff_read, &header);
  if(rc == -1){
    puts("Error reading frame");
    return -1;
  }
  switch(header.type){
      case DATA_TYPE://Data
          printf("TODO: Data Frame. Not implemented yet.");
          return -1;
      case HEADERS_TYPE://Header
          printf("TODO: Header Frame. Not implemented yet.");
          return -1;
      case PRIORITY_TYPE://Priority
          printf("TODO: Priority Frame. Not implemented yet.");
          return -1;
      case RST_STREAM_TYPE://RST_STREAM
          printf("TODO: Reset Stream Frame. Not implemented yet.");
          return -1;
      case SETTINGS_TYPE:{//Settings
          rc = read_settings_payload(buff_read, &header);
          if(rc == -1){
            puts("Error in read settings payload");
            return -1;
          }
          return rc;
      }
      case PUSH_PROMISE_TYPE://Push promise
          printf("TODO: Push promise frame. Not implemented yet.");
          return -1;
      case PING_TYPE://Ping
          printf("TODO: Ping frame. Not implemented yet.");
          return -1;
      case GOAWAY_TYPE://Go Avaw
          printf("TODO: Go away frame. Not implemented yet.");
          return -1;
      case WINDOW_UPDATE_TYPE://Window update
          printf("TODO: Window update frame. Not implemented yet.");
          return -1;
      case CONTINUATION_TYPE://Continuation
          printf("TODO: Continuation frame. Not implemented yet.");
          return -1;
      default:
          printf("Error: Type not found");
          return -1;
  }
}
