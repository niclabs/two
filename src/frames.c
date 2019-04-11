
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
int bytesToFrameHeader(uint8_t* byte_array, int size, frameheader_t* frame_header){
    if(size < 9){
        printf("ERROR: frameHeader size too small, %d\n", size);
        return -1;
    }
    frame_header->length = bytesToUint32_24(byte_array);
    frame_header->type = (uint8_t)(byte_array[3]);
    frame_header->flags = (uint8_t)(byte_array[4]);
    frame_header->stream_id = bytesToUint32_31(byte_array+5);
    frame_header->reserved = (uint8_t)((byte_array[5])>>7);
    return 0;
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
        uint8_t setting_bytes[6];
        int size = settingToBytes(settings_frame->pairs+i, setting_bytes);
        for(int j = 0; j<size; j++){
            bytes[i*6+j] = setting_bytes[j];
        }
        
    }
    return 6*count;
}

/*transforms an array of bytes to a settings frame*/
int bytesToSettingsPayload(uint8_t* bytes, int size, settingsframe_t* settings_frame, settingspair_t* pairs){
    if(size%6!=0){
        printf("ERROR: settings payload wrong size\n");
        return -1;
    }
    int count = size / 6;
    
    for(int i = 0; i< count; i++){
        pairs[i].identifier = bytesToUint16(bytes+(i*6));
        pairs[i].value = bytesToUint32(bytes+(i*6)+2);
    }

    settings_frame->pairs = pairs;
    settings_frame->count = count;
    return (6*count);
}


/*
uint8_t getTypeFromBytes(uint8_t * bytes, uint8_t bytes_length){

}
uint8_t getLengthFromBytes(uint8_t * bytes, uint8_t bytes_length){

}
*/

/*
int bytesToFrame(uint8_t * bytes, int size, frame_t* frame){
    frameheader_t frame_header;
    int not_ok = bytesToFrameHeader(bytes, size, &frame_header);
    if(!not_ok){
        if(size<(9+(int)frame_header.length)){
            printf("FramePayload is still not complete");
            return -1;
        }

        switch(frame_header.type){
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
                settingsframe_t settings_frame;
                settingspair_t pairs[frame_header.length/6];
                int size = bytesToSettingsFrame(bytes+9,frame_header.length, &settings_frame, pairs);
                frame->frame_header = frame_header;
                frame->payload = (void*)&settings_frame;//TODO FIX this!!! assigning settings frame here but ussing it in upper layer...
                return 9+size;
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
    return -1;
}
*/
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
        case 0x4: {//Settings
            if (length % 6 != 0) {
                printf("Error: length not divisible by 6, %d", length);
                return -1;
            }
            
            uint8_t frame_header_bytes[9];


            int frame_header_bytes_size = frameHeaderToBytes(frame_header, frame_header_bytes);

            settingsframe_t *settings_frame = ((settingsframe_t *) (frame->payload));

            
            uint8_t settings_bytes[length];

            int size = settingsFrameToBytes(settings_frame, length / 6, settings_bytes);
            int new_size = appendByteArrays(bytes, frame_header_bytes, settings_bytes, frame_header_bytes_size, size);
            //TODO delete frame_header_bytes and frame_header_bytes?
            //free(frame_header_bytes);
            //free(settings_bytes);
            //free(frame_header_bytes);
            //free(frame_header_byte_array);

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

int createListOfSettingsPair(uint16_t* ids, uint32_t* values, int count, settingspair_t* settings_pairs){
    for (int i = 0; i < count; i++){
        settings_pairs[i].identifier = ids[i];
        settings_pairs[i].value = values[i];
    }
    return count;
}


int createSettingsFrame(uint16_t* ids, uint32_t* values, int count, frame_t* frame, frameheader_t* frame_header, settingsframe_t* settings_frame, settingspair_t* pairs){
    frame_header->length = count*6;
    frame_header->type = 0x4;//settings;
    frame_header->flags = 0x0;
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    count = createListOfSettingsPair(ids, values, count, pairs);
    settings_frame->count=count;
    settings_frame->pairs= pairs;
    frame->payload = (void*)settings_frame;
    frame->frame_header = frame_header;
    return 0;
}
int createSettingsAckFrame(frame_t * frame, frameheader_t* frame_header){
    frame_header->length = 0;
    frame_header->type = 0x4;//settings;
    frame_header->flags = setFlag(0x0, SETTINGS_ACK_FLAG);
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    frame->frame_header = frame_header;
    frame->payload = NULL;
    return 0;
}



int isFlagSet(uint8_t flags, uint8_t queried_flag){
    if((queried_flag&flags) >0){
        return 1;
    }
    return 0;
}

uint8_t setFlag(uint8_t flags, uint8_t flag_to_set){
    uint8_t new_flag = flags|flag_to_set;
    return new_flag;
}