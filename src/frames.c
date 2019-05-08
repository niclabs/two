
#include "frames.h"

/*
* Function: frame_header_to_bytes
* Pass a frame header to an array of bytes
* Input: pointer to frameheader, array of bytes
* Output: 0 if bytes were read correctly, (-1 if any error reading)
*/
int frame_header_to_bytes(frame_header_t *frame_header, uint8_t *byte_array){
    uint8_t length_bytes[3];
    uint32_24_to_byte_array(frame_header->length, length_bytes);
    uint8_t stream_id_bytes[4];
    uint32_31_to_byte_array(frame_header->stream_id, stream_id_bytes);
    for(int i =0; i<3;i++){
        byte_array[0+i] = length_bytes[i];   //length 24
    }
    byte_array[3] = (uint8_t)frame_header->type; //type 8
    byte_array[4] = (uint8_t)frame_header->flags; //flags 8
    for(int i =0; i<4;i++){
        byte_array[5+i] = stream_id_bytes[i];//stream_id 31
    }
    byte_array[5] = byte_array[5]|(frame_header->reserved<<7); // reserved 1
    return 9;
}

/*
* Function: bytes_to_frame_header
* Transforms bytes to a FrameHeader
* Input:  array of bytes, size ob bytes to read,pointer to frameheader
* Output: 0 if bytes were written correctly, -1 if byte size is <9
*/
int bytes_to_frame_header(uint8_t *byte_array, int size, frame_header_t *frame_header){
    if(size < 9){
        printf("ERROR: frameHeader size too small, %d\n", size);
        return -1;
    }
    frame_header->length = bytes_to_uint32_24(byte_array);
    frame_header->type = (uint8_t)(byte_array[3]);
    frame_header->flags = (uint8_t)(byte_array[4]);
    frame_header->stream_id = bytes_to_uint32_31(byte_array+5);
    frame_header->reserved = (uint8_t)((byte_array[5])>>7);
    return 0;
}

/*
* Function: setting_to_bytes
* Pass a settingPair to bytes
* Input:  settingPair, pointer to bytes
* Output: size of written bytes
*/
int setting_to_bytes(settings_pair_t *setting, uint8_t *bytes){
    uint8_t identifier_bytes[2];
    uint16_to_byte_array(setting->identifier, identifier_bytes);
    uint8_t value_bytes[4];
    uint32_to_byte_array(setting->value, value_bytes);
    for(int i = 0; i<2; i++){
        bytes[i] = identifier_bytes[i];
    }
    for(int i = 0; i<4; i++){
        bytes[2+i] = value_bytes[i];
    }
    return 6;
}

/*
* Function: settings_frame_to_bytes
* pass a settings payload to bytes
* Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
* Output: size of written bytes
*/
int settings_frame_to_bytes(settings_payload_t *settings_payload, uint32_t count, uint8_t *bytes){
    for(uint32_t  i = 0; i< count; i++){
        //printf("%d\n",i);
        uint8_t setting_bytes[6];
        int size = setting_to_bytes(settings_payload->pairs+i, setting_bytes);
        for(int j = 0; j<size; j++){
            bytes[i*6+j] = setting_bytes[j];
        } 
    }
    return 6*count;
}

/*
* Function: bytes_to_settings_payload
* pass a settings payload to bytes
* Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
* Output: size of written bytes
*/
int bytes_to_settings_payload(uint8_t *bytes, int size, settings_payload_t *settings_payload, settings_pair_t *pairs){
    if(size%6!=0){
        printf("ERROR: settings payload wrong size\n");
        return -1;
    }
    int count = size / 6;

    for(int i = 0; i< count; i++){
        pairs[i].identifier = bytes_to_uint16(bytes+(i*6));
        pairs[i].value = bytes_to_uint32(bytes+(i*6)+2);
    }

    settings_payload->pairs = pairs;
    settings_payload->count = count;
    return (6*count);
}

/*
* Function: frame_to_bytes
* pass a complete Frame(of any type) to bytes
* Input:  Frame pointer, pointer to bytes
* Output: size of written bytes, -1 if any error
*/
int frame_to_bytes(frame_t *frame, uint8_t *bytes){
    frame_header_t* frame_header = frame->frame_header;
    uint32_t length = frame_header->length;
    uint8_t type = frame_header->type;

    switch(type){
        case 0x0://Data
            printf("TODO: Data Frame. Not implemented yet.");

            return -1;
        case 0x1://Header
            printf("TODO: Header Frame. Not implemented yet.");
            return -1;
        case 0x2://Priority
            printf("TODO: Priority Frame. Not implemented yet.");
            return -1;
        case 0x3://RST_STREAM
            printf("TODO: Reset Stream Frame. Not implemented yet.");
            return -1;
        case 0x4: {//Settings
            if (length % 6 != 0) {
                printf("Error: length not divisible by 6, %d", length);
                return -1;
            }

            uint8_t frame_header_bytes[9];


            int frame_header_bytes_size = frame_header_to_bytes(frame_header, frame_header_bytes);

            settings_payload_t *settings_payload = ((settings_payload_t *) (frame->payload));


            uint8_t settings_bytes[length];

            int size = settings_frame_to_bytes(settings_payload, length / 6, settings_bytes);
            int new_size = append_byte_arrays(bytes, frame_header_bytes, settings_bytes, frame_header_bytes_size, size);
            return new_size;
        }
        case 0x5://Push promise
            printf("TODO: Push promise frame. Not implemented yet.");
            return -1;
        case 0x6://Ping
            printf("TODO: Ping frame. Not implemented yet.");
            return -1;
        case 0x7://Go Avaw
            printf("TODO: Go away frame. Not implemented yet.");
            return -1;
        case 0x8://Window update
            printf("TODO: Window update frame. Not implemented yet.");
            return -1;
        case 0x9://Continuation
            printf("TODO: Continuation frame. Not implemented yet.");
            return -1;
        default:
            printf("Error: Type not found");
            return -1;
    }


}


/*
* Function: create_list_of_settings_pair
* Create a List Of SettingsPairs
* Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to settings Pairs
* Output: size of read setting pairs
*/
int create_list_of_settings_pair(uint16_t *ids, uint32_t *values, int count, settings_pair_t *settings_pairs){
    for (int i = 0; i < count; i++){
        settings_pairs[i].identifier = ids[i];
        settings_pairs[i].value = values[i];
    }
    return count;
}

/*
* Function: create_settings_frame
* Create a Frame of type sewttings with its payload
* Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to frame, pointer to frameheader, pointer to settings payload, pointer to settings Pairs.
* Output: 0 if setting frame was created
*/
int create_settings_frame(uint16_t *ids, uint32_t *values, int count, frame_t *frame, frame_header_t *frame_header,
                          settings_payload_t *settings_payload, settings_pair_t *pairs){
    frame_header->length = count*6;
    frame_header->type = 0x4;//settings;
    frame_header->flags = 0x0;
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    count = create_list_of_settings_pair(ids, values, count, pairs);
    settings_payload->count=count;
    settings_payload->pairs= pairs;
    frame->payload = (void*)settings_payload;
    frame->frame_header = frame_header;
    return 0;
}

/*
* Function: create_settings_ack_frame
* Create a Frame of type sewttings with flag ack 
* Input:  pointer to frame, pointer to frameheader
* Output: 0 if setting frame was created
*/
int create_settings_ack_frame(frame_t *frame, frame_header_t *frame_header){
    frame_header->length = 0;
    frame_header->type = 0x4;//settings;
    frame_header->flags = set_flag(0x0, SETTINGS_ACK_FLAG);
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    frame->frame_header = frame_header;
    frame->payload = NULL;
    return 0;
}


/*
* Function: is_flag_set
* Tells if a flag is set in a flag byte.
* Input:  flag byte to test, queried flag
* Output: 1 if queried flag was set, 0 if not
*/
int is_flag_set(uint8_t flags, uint8_t queried_flag){
    if((queried_flag&flags) >0){
        return 1;
    }
    return 0;
}


/*
* Function: set_flag
* Sets a flag in a flag byte.
* Input: flag byte, flag to set
* Output: flag byte with flag setted
*/
uint8_t set_flag(uint8_t flags, uint8_t flag_to_set){
    uint8_t new_flag = flags|flag_to_set;
    return new_flag;
}
