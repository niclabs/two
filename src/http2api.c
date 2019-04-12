#include "http2api.h"
/*
* This API assumes the existence of a TCP connection between Server and Client,
* and two methods to use this TCP connection that are tcp_write and tcp_write
*/

/*--- Global Variables ---*/
uint32_t remote_settings[6];
uint32_t local_settings[6];
uint32_t local_cache[6];
uint8_t client;
uint8_t server;
uint8_t waiting_sett_ack;

/*-----------------------*/

/*
* Function: send_local_settings
* Sends local settings to endpoint
* Input: void
* Output: 0 if settings were sent. 1 if not.
*/
uint8_t send_local_settings(void){
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
    return 1;
    }
  uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
  int size_byte_mysettings = frameToBytes(&mysettingframe, byte_mysettings);
  /*Assuming that tcp_write returns the number of bytes written*/
  rc = tcp_write(byte_mysettings, size_byte_mysettings);
  if(rc != size_byte_mysettings){
    puts("Error in local settings sending");
    return 1;
  }
  return 0;
}

/*
* Function: update_remote_settings
* Updates the table where remote settings are stored
* Input: -> sframe, it must be a SETTINGS frame
*        -> place, must be LOCAL or REMOTE. It indicates which table to update.
* Output: 0 if update was successfull, 1 if not
*/
uint8_t update_settings_table(frame_t* sframe, uint8_t place){
  /*Esta parte no debería ir si cambiamos la forma de trabajar con los frames*/
  if(sframe->frame_header->type != 0x4){
    puts("Error: frame is not a SETTTINGS frame");
    return 1;
  }
  /*spl is for setttings payload*/
  settingspayload_t *spl = (settingspayload_t *)sframe->payload;
  uint8_t i;
  uint16_t id;
  if(place == REMOTE){
    /*Update remote table*/
    for(i = 0; i < spl->count; i++){
      id = spl->pairs[i].identifier;
      if(id < 1 || id > 6){
        puts("Error: setting identifier not valid");
        return 1;
      }
      remote_settings[--id] = spl->pairs[i].value;
    }
    return 0;
  }
  else if(place == LOCAL){
    /*Update local table*/
    for(i = 0; i < spl->count; i++){
      id = spl->pairs[i].identifier;
      if(id < 1 || id > 6){
        puts("Error: setting identifier not valid");
        return 1;
      }
      local_settings[--id] = spl->pairs[i].value;
    }
    return 0;
  }
  else{
    puts("Error: Not a valid table to update");
    return 1;
  }
}

/*
* Function: send_settings_ack
* Sends an ACK settings frame to endpoint
* Input: void
* Output: 0 if sent was successfully made, 1 if not.
*/
uint8_t send_settings_ack(void){
  frame_t ack_frame;
  frameheader_t ack_frame_header;
  uint8_t rc;
  createSettingsAckFrame(&ack_frame, &ack_frame_header);
  uint8_t byte_ack[9+0]; /*Settings ACK frame only has a header*/
  int size_byte_ack = frameToBytes(&ack_frame, byte_ack);
  /*TODO: tcp_write*/
  rc = tcp_write(byte_ack, size_byte_ack)
  if(rc != size_byte_ack){
    puts("Error in Settings ACK sending");
    return 1;
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
* Output: 0 if connection was made successfully. 1 if not.
*/
uint8_t init_connection(void){
  uint8_t preface_buff[24];
  uint8_t rc;
  if(client){
    puts("Client sends preface");
    char *preface = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";
    uint8_t i = 0;
    /*We load the buffer with the ascii characters*/
    while(preface[i] != '\0'){
      i=i+1;//not sure if before of after assignment
      preface_buff[i] = preface[i];
    }
    if(!(rc = tcp_write(preface_buff,24))){
      puts("Error in preface sending");
      return 1;
    }
    if(!(rc = send_local_settings())){
      puts("Error in local settings sending");
      return 1;
    }
    return 0;
  }
  else if(server){
    puts("Server waits for preface");
    /*TODO server wait for preface*/

  }

  return 5;//fix this XD
}
