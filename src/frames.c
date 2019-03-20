
#include "frames.h"

/*pass a frame header to an array of bytes*/
int frameHeaderToBytes(frameheader_t* frame_header, uint8_t* byte_array){


    uint8_t* length_bytes = uint32_24toByteArray(frame_header->length);
    uint8_t* stream_id_bytes = uint32_31toByteArray(frame_header->stream_id);

    for(int i =0; i<3;i++){
        byte_array[0+i] = length_bytes[i];   //length 24

    }
    free(length_bytes);
    byte_array[3] = (uint8_t)frame_header->type; //type 8

    byte_array[4] = (uint8_t)frame_header->flags; //flags 8

    
    for(int i =0; i<4;i++){
        byte_array[5+i] = stream_id_bytes[i];//stream_id 31
    }
    free(stream_id_bytes);
    byte_array[5] = byte_array[5]|(frame_header->reserved<<7); // reserved 1



    return 9;
}

/*Transforms bytes to a FrameHeader*/
frameheader_t* bytesToFrameHeader(uint8_t* bytes, int size ){
    if(size < 9){
        printf("ERROR: frameHeader size too small, %d\n", size);
        return NULL;
    }
    frameheader_t* frame_header = (frameheader_t*) malloc(sizeof(frameheader_t));
    frame_header->length = bytesToUint32_24(bytes);
    frame_header->type = (uint8_t)(bytes[3]);
    frame_header->flags = (uint8_t)(bytes[4]);
    frame_header->stream_id = bytesToUint32_31(bytes+5);
    frame_header->reserved = (uint8_t)((bytes[5])>>7);
    return frame_header;
}



/*pass a settingPair to bytes*/
int settingToBytes(settingspair_t* setting, uint8_t* bytes){
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
    return 6;
}
/*pass a settings frame to bytes*/
int settingsFrameToBytes(settingsframe_t* settings_frame, uint32_t count, uint8_t* bytes){
    for(uint32_t  i = 0; i< count; i++){
        //printf("%d\n",i);
        uint8_t* setting_bytes = (uint8_t*)malloc(6);
        int size = settingToBytes(settings_frame->pairs+i, setting_bytes);
        for(int j = 0; j<size; j++){
            bytes[i*6+j] = setting_bytes[j];
        }
        free(setting_bytes);
    }
    return 6*count;
}

/*transforms an array of bytes to a settings frame*/
settingsframe_t* bytesToSettingsFrame(uint8_t* bytes, int size){
    if(size%6!=0){
        printf("ERROR: settings payload wrong size\n");
        return NULL;
    }
    int count = size / 6;
    settingspair_t* settings = (settingspair_t*)malloc(sizeof(settingspair_t)*6);
    for(int i = 0; i< count; i++){
        settings[i] =  (settingspair_t){bytesToUint16(bytes+(i*6)), bytesToUint32(bytes+(i*6)+2)};
        //printf("id %d, val %d\n", settings[i].identifier, settings[i].value);
        //printf("bytes id %u, %u\n",bytes[6*i], bytes[6*i+1]);
    }
    settingsframe_t* frame = (settingsframe_t*)malloc(sizeof(settingsframe_t));
    frame->pairs = settings;
    return frame;
}


/*
uint8_t getTypeFromBytes(uint8_t * bytes, uint8_t bytes_length){

}
uint8_t getLengthFromBytes(uint8_t * bytes, uint8_t bytes_length){

}
*/


frame_t* bytesToFrame(uint8_t * bytes, int size){
    frameheader_t *frame_header= bytesToFrameHeader(bytes, size);
    if(frame_header){
        if(size<(9+(int)frame_header->length)){
            printf("FramePayload is still not complete");
            return NULL;
        }
        switch(frame_header->type){
            case 0x0://Data
                printf("TODO: Data Frame. Not implemented yet.");
                return NULL;
            case 0x1://Header
                printf("TODO: Header Frame. Not implemented yet.");
                return NULL;
            case 0x2://Priority
                printf("TODO: Priority Frame. Not implemented yet.");
                return NULL;
            case 0x3://RST_STREAM
                printf("TODO: Reset Stream Frame. Not implemented yet.");
                return NULL;
            case 0x4:{//Settings
                settingsframe_t* settings_frame = bytesToSettingsFrame(bytes+9,frame_header->length);
                frame_t* frame = (frame_t*)malloc(sizeof(frame_t));
                frame->frame_header = frame_header;
                frame->payload = (void*)settings_frame;
                return frame;}
            case 0x5://Push promise
                printf("TODO: Push promise frame. Not implemented yet.");
                return NULL;
            case 0x6://Ping
                printf("TODO: Ping frame. Not implemented yet.");
                return NULL;
            case 0x7://Go Avaw
                printf("TODO: Go away frame. Not implemented yet.");
                return NULL;
            case 0x8://Window update
                printf("TODO: Window update frame. Not implemented yet.");
                return NULL;
            case 0x9://Continuation
                printf("TODO: Continuation frame. Not implemented yet.");
                return NULL;
            default:
                printf("Error: Type not found");
                return NULL;
        }

    }
    return NULL;


}
int frameToBytes(frame_t* frame, uint8_t* bytes){
    frameheader_t* frame_header = frame->frame_header;
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
        case 0x4://Settings
            if(length%6!=0){
                printf("Error: length not divisible by 6, %d", length);
                return -1;
            }
            uint8_t* frame_header_bytes = (uint8_t*)malloc(sizeof(uint8_t)*9);;
            int frame_header_bytes_size = frameHeaderToBytes(frame_header, frame_header_bytes);

            settingsframe_t* settings_frame = ((settingsframe_t*)(frame->payload));

            uint8_t* settings_bytes = (uint8_t*)malloc(sizeof(uint8_t)*length);
            int size = settingsFrameToBytes(settings_frame, length/6, settings_bytes);
            int new_size = appendByteArrays(bytes, frame_header_bytes, settings_bytes, frame_header_bytes_size, size);
            //TODO delete frame_header_bytes and frame_header_bytes?
            //free(frame_header_bytes);
            //free(settings_bytes);
            //free(frame_header_bytes);
            //free(frame_header_byte_array);

            return new_size;
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

int createListOfSettingsPair(uint16_t* ids, uint32_t* values, int count, settingspair_t* settings_pairs){
    for (int i = 0; i < count; i++){
        settings_pairs[i].identifier = ids[i];
        settings_pairs[i].value = values[i];
    }
    return count;
}

frameheader_t* createFrameHeader(uint32_t length, uint8_t type, uint8_t flags, uint8_t reserved, uint32_t stream_id){
    frameheader_t* frame_header = (frameheader_t*)malloc(sizeof(frameheader_t));
    frame_header->length = length;
    frame_header->type = type;
    frame_header->flags = flags;
    frame_header->reserved = reserved;
    frame_header->stream_id = stream_id;
    return frame_header;
}


frame_t* createFrame(frameheader_t* frame_header, void* payload){
    frame_t* frame = (frame_t*)malloc(sizeof(frame_t));
    frame->frame_header = frame_header;
    frame->payload = payload;
    return frame;
}

frame_t* createSettingsFrame(uint16_t* ids, uint32_t* values, int count){

    int length = count*6; //&

    int type = 0x4;//settings
    int flags = 0x0;
    int reserved = 0x0;
    int stream_id = 0;
    frameheader_t *frame_header = createFrameHeader(length, type, flags, reserved, stream_id);
    settingspair_t *settings_pairs = (settingspair_t*)malloc(sizeof(settingspair_t)*count);
    count = createListOfSettingsPair(ids, values, count, settings_pairs);
    settingsframe_t* settings_frame = (settingsframe_t*)malloc(sizeof(settingsframe_t));
    settings_frame->pairs = settings_pairs;
    settings_frame->count = count;
    frame_t *frame = createFrame(frame_header,(void*)settings_frame);
    return frame;
}

frame_t* createSettingsAckFrame(void){
    int length = 0;
    int type = 0x4;//settings
    int flags = 0x1;//ack
    int reserved = 0x0;
    int stream_id = 0;
    frameheader_t *frame_header = createFrameHeader(length, type, flags, reserved, stream_id);
    frame_t *frame = createFrame(frame_header,NULL);
    return frame;
}
