
#include "frames.h"

/*pass a frame header to an array of bytes*/
uint8_t* frameHeaderToBytes(frameheader_t* frame_header){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*9);

    uint8_t* length_bytes = uint32_24toByteArray(frame_header->length);
    uint8_t* stream_id_bytes = uint32_31toByteArray(frame_header->stream_id);

    for(int i =0; i<3;i++){
        bytes[0+i] = length_bytes[i];   //length 24

    }
    free(length_bytes);
    bytes[3] = (uint8_t)frame_header->type; //type 8

    bytes[4] = (uint8_t)frame_header->flags; //flags 8

    
    for(int i =0; i<4;i++){
        bytes[5+i] = stream_id_bytes[i];//stream_id 31
    }
    free(stream_id_bytes);
    bytes[5] = bytes[5]|(frame_header->reserved<<7); // reserved 1

    return bytes;
}

/*Transforms bytes to a FrameHeader*/
frameheader_t* bytesToFrameHeader(uint8_t* bytes, uint8_t length ){
    if(length<9){
        printf("ERROR: frameHeader size too small, %d\n", length);
        return NULL;
    }
    frameheader_t* frame_header = (frameheader_t*) malloc(sizeof(frameheader_t));
    frame_header->length = bytesToUint32_24(bytes);
    frame_header->type = (uint8_t)(bytes[3]);
    frame_header->flags = (uint8_t)(bytes[4]);
    frame_header->stream_id = bytesToUint32_31(bytes+5);
    frame_header->reserved = (bool)((bytes[5])>>7);    
    return frame_header;
}



/*pass a settingPair to bytes*/
uint8_t* settingToBytes(settingspair_t* setting){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*6);
    uint8_t* identifier_bytes = uint16toByteArray(setting->identifier);     
    uint8_t* value_bytes = uint32toByteArray(setting->value);
    for(int i = 0; i<2; i++){
        bytes[i] = identifier_bytes[i];    
    }
    free(identifier_bytes);
    for(int i = 0; i<4; i++){
        bytes[2+i] = value_bytes[i];
    }
    free(value_bytes);
    return bytes;
}
/*pass a settings frame to bytes*/
uint8_t* settingsFrameToBytes(settingsframe_t* settings_frame, uint8_t count){
    uint8_t* bytes = (uint8_t*)malloc(sizeof(uint8_t)*6*count);
    for(int i = 0; i< count; i++){
        uint8_t* setting_bytes = settingToBytes(&settings_frame->settings[i]);
        for(int j = 0; j<6; j++){
            bytes[i*6+j] = setting_bytes[j];
        }
        free(setting_bytes);
    }
    return bytes;
}

/*transforms an array of bytes to a settings frame*/
settingsframe_t* bytesToSettingsFrame(uint8_t* bytes, uint8_t length){
    if(length%6!=0){
        printf("ERROR: settings payload wrong size\n");
        return NULL;
    }
    int count = length / 6;
    settingspair_t* settings = (settingspair_t*)malloc(sizeof(settingspair_t)*6);
    for(int i = 0; i< count; i++){
        settings[i] =  (settingspair_t){bytesToUint16(bytes+(i*6)), bytesToUint32(bytes+(i*6)+2)};
        //printf("id %d, val %d\n", settings[i].identifier, settings[i].value);
        //printf("bytes id %u, %u\n",bytes[6*i], bytes[6*i+1]);
    }
    settingsframe_t* frame = (settingsframe_t*)malloc(sizeof(settingsframe_t));
    frame->settings = settings;
    return frame;
}


