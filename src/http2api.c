#include "http2api.h"
/*
* This API assumes the existence of a TCP connection between Server and Client,
* and two methods to use this TCP connection that are tcp_write and tcp_write
* The methods starting with "server" and "client" are of server and client side
* respectively.
*/

/*--- Global Variables ---*/
uint32_t remote_settings[7];
uint32_t local_settings[7];
uint32_t local_cache[7];
uint8_t client;
uint8_t server;
uint8_t waiting_sett_ack;

/*-----------------------*/

/*
* Function: send_local_settings
* Sends local settings to endpoint
* Input: void
* Output: A positive number if settings were sent. 0 if not.
*/
uint8_t send_local_settings(void){
  uint8_t rc;
  uint16_t ids[6] = {0x1, 0x2, 0x3, 0x4, 0x5, 0x6};
  frame_t *mysettings = createSettingsFrame(ids, &local_settings[1], 6);
  uint8_t byte_mysettings[9+6*6]; /*header: 9 bytes + 6 * setting: 6 bytes */
  int size_byte_mysettings = frameToBytes(mysettings, byte_mysettings);
  /*Assuming that tcp_write returns the number of bytes written*/
  if(!(rc = tcp_write(byte_mysettings))){
    puts("Error in local settings sending");
    return rc;
  }
  return rc;
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
  settingsframe_t *spl = (settingsframe_t *)sframe->payload;
  uint8_t i;
  for(i = 0; i < spl->count; i++){
    uint16_t id = spl->pairs[i].identifier;
    if(id > 6 | id < 1){
      puts("Error: setting identifier not valid");
      return 1;
    }
    if(place == REMOTE){
      remote_settings[id] = spl->pairs[i].value;
    }
    else if(place == LOCAL){
      local_settings[id] = spl->pairs[i].value;
    }
    else{
      puts("Error: Not a valid table to update");
      return 1;
    }
  }
  return 0;

}

/*
* Function: send_settings_ack
* Sends an ACK settings frame to endpoint
* Input: void
* Output: A positive number if ACK settings were sent, 0 if not.
*/
uint8_t send_settings_ack(void){
  frame_t * ack_frame = createSettingsAckFrame();
  /*Settings ACK frame only has a header*/
  uint8_t byte_ack[9+0];
  int size_byte_ack = frameToBytes(ack_frame, byte_ack);
  uint8_t rc;
  /*TODO: tcp_write*/
  if(!(rc = tcp_write(byte_ack))){
    puts("Error in Settings ACK sending");
    return rc;
  }
  return rc;
}

/*
* Function: read_setting_from
* Reads a setting parameter from local or remote table
* Input: -> place: must be LOCAL or REMOTE. It indicates the table to read.
*        -> param: it indicates which parameter to read from table.
* Output: The value read from the table. 0 if nothing was read.
*/

uint32_t read_setting_from(uint8_t place, uint8_t param){
  if(param < 1 | param > 6){
    printf("Error: %u is not a valid setting parameter", param);
    return 0;
  }
  if(place == LOCAL){
    return local_settings[param];
  }
  else if(place == REMOTE){
    return remote_settings[param];
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
      preface_buff[i++] = preface[i++];
    }
    if(!(rc = tcp_write(preface_buff))){
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
}
