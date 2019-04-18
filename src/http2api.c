#include "http2utils.h"

/*
* This API assumes the existence of a TCP connection between Server and Client,
* and two methods to use this TCP connection that are tcp_write and tcp_write
*/

/*--- Global Variables ---*/
static uint32_t remote_settings[6];
static uint32_t local_settings[6];
//static uint32_t local_cache[6];
static uint8_t client = 0;
static uint8_t server = 0;
//static uint8_t waiting_sett_ack = 0;

/*-----------------------*/
/*
* Function: init_server
* Initialize variables for server
* Input: void
* Output: 0 if initialize were made. -1 if not.
*/
int init_server(void){
  if(client){
    puts("Error: this is a client process");
    return -1;
  }
  if(server){
    puts("Error: server was already initalized");
    return -1;
  }
  remote_settings[0] = local_settings[0] = DEFAULT_HTS;
  remote_settings[1] = local_settings[1] = DEFAULT_EP;
  remote_settings[2] = local_settings[2] = DEFAULT_MCS;
  remote_settings[3] = local_settings[3] = DEFAULT_IWS;
  remote_settings[4] = local_settings[4] = DEFAULT_MFS;
  remote_settings[5] = local_settings[5] = DEFAULT_MHLS;
  server = 1;
  return 0;
}

/*
* Function: init_client
* Initialize variables for client
* Input: void
* Output: 0 if initialize were made. -1 if not.
*/
int init_client(void){
  if(server){
    puts("Error: this is a server process");
    return -1;
  }
  if(client){
    puts("Error: client was already initalized");
    return -1;
  }
  remote_settings[0] = local_settings[0] = DEFAULT_HTS;
  remote_settings[1] = local_settings[1] = DEFAULT_EP;
  remote_settings[2] = local_settings[2] = DEFAULT_MCS;
  remote_settings[3] = local_settings[3] = DEFAULT_IWS;
  remote_settings[4] = local_settings[4] = DEFAULT_MFS;
  remote_settings[5] = local_settings[5] = DEFAULT_MHLS;
  client = 1;
  return 0;
}

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
  if(!rc){
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
* Function: init_connection
* Input: void
* Output: 0 if connection was made successfully. -1 if not.
*/
int init_connection(void){
  int rc;
  char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
  if(client){
    uint8_t preface_buff[24];
    puts("Client sends preface");
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
    if((rc = send_local_settings()) < 0){
      puts("Error in local settings sending");
      return -1;
    }
    return 0;
  }
  else if(server){
    uint8_t preface_buff[25];
    preface_buff[24] = '\0';
    int read_bytes = 0;
    puts("Server waits for preface");
    /*We read the first 24 byes*/
    while(read_bytes < 24){
      /*Assuming that tcp_read returns the number of bytes read*/
      read_bytes = read_bytes + tcp_read(preface_buff+read_bytes, 24 - read_bytes);
    }
    if(strcmp(preface, (char*)preface_buff) != 0){
      puts("Error in preface receiving");
      return -1;
    }
    if((rc = send_local_settings()) < 0){
      puts("Error in local settings sending");
      return -1;
    }
    return 0;
  }
  return -1;
}


/*
*
*
*
*/
int read_frame(uint8_t *buff_read, frameheader_t *header){
  int read_bytes = 0;
  while(read_bytes < 9){
    read_bytes = read_bytes + tcp_read(buff_read+read_bytes, 9 - read_bytes);
  }
  int rc = bytesToFrameHeader(buff_read, 9, header);
  if(rc != 9){
    puts("Error coding bytes to frame header");
    return -1;
  }
  read_bytes = 0;
  if(header->length > 256){
    puts("Error: Payloadsize too big");
    return -1;
  }
  while(read_bytes < header->length){
    read_bytes = read_bytes + tcp_read(buff_read+read_bytes, header->length - read_bytes);
  }
  return 0;
}
/*
*
*
*
*/
int read_settings_frame(uint8_t *buff_read, frameheader_t *header){
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
*
*/
int wait(void){
  uint8_t buff_read[MAX_BUFFER_SIZE];
  uint8_t buff_write[MAX_BUFFER_SIZE];
  int read_bytes;
  int rc = init_connection();
  if(rc){
    puts("Error in init connect");
    return -1;
  }
  while(1){
    frameheader_t header;
    rc = read_frame(buff_read, &header);
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
            read_settings_frame(buff_read, &header);
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
}
