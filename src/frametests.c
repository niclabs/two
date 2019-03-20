
//#include "frames.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "frames.h"



/*tests if trnasforming from frameHeader to bytes (and vice versa) works*/
int frameHeaderEncodeDecodeTest(int argc, char **argv){


    if (argc < 6){
        printf("usage: %s <length> <type> <flags> <reserved> <stream_id>\n", argv[0]);
        return 1;
    }

    int length = (uint32_t)atoi(argv[1]);
    int type = (uint8_t)atoi(argv[2]);
    int flags = (uint8_t)atoi(argv[3]);
    int reserved = (uint8_t)atoi(argv[4]);
    int stream_id = (uint32_t)atoi(argv[5]);


    printf("TEST: frameHeaderEncodeDecodeTest\n");
    frameheader_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t* frame_bytes = (uint8_t*)malloc(9);
    int frame_bytes_size = frameHeaderToBytes(&frame_header, frame_bytes);
    frameheader_t* decoder_frame_header = bytesToFrameHeader(frame_bytes, frame_bytes_size);
    
    if(frame_header.length!=decoder_frame_header->length){
        printf("ERROR: length don't match\n");
        return -1;
    }
    if(frame_header.flags!=decoder_frame_header->flags){
        printf("ERROR: flags don't match\n");
        return -1;
    }
    if(frame_header.type!=decoder_frame_header->type){
        printf("ERROR: type don't match\n");
        return -1;
    }
    if(frame_header.stream_id!=decoder_frame_header->stream_id){
        printf("ERROR: stream_id don't match\n");
        return -1;
    }
    if(frame_header.reserved!=decoder_frame_header->reserved){
        printf("ERROR: reserved don't match\n");
        return -1;
    }
    printf("frameHeaderEncodeDecodeTest OK\n");
    return 0;
}

/*tests if trnasforming from settingsframe to bytes (and vice versa) works*/
int settingEncodeDecodeTest(int argc, char **argv){
    printf("TEST: settingEncodeDecodeTest\n");


    if ((argc-1)%2!=0){
        printf("usage: %s <id_1> <value_1> ... <id_i> <value_i>\n", argv[0]);
        return 1;
    }
    int count = argc-1;


    uint16_t ids[count/2];
    uint32_t values[count/2];
    for (int i = 0; i < count/2; i++){
        ids[i] = (uint16_t)atoi(argv[2*i]+1);
        values[i] = (uint32_t)atoi(argv[2*i+1+1]);
    }


    settingspair_t  settings[count/2];
    for (int i = 0; i < count/2; i++){
        settingspair_t pair = {ids[i],values[i]};
        settings[i] = pair;
    }


    settingsframe_t frame_settings = {(settingspair_t *) &settings, count/2};

    uint8_t *setting_frame_bytes = (uint8_t*)malloc(count/2*6);
    int size = settingsFrameToBytes(&frame_settings, count/2, setting_frame_bytes);

    settingsframe_t *decoded_settings = bytesToSettingsFrame(setting_frame_bytes, 6*count/2);

    for (int i = 0; i < size/6; i++) {
        if (decoded_settings->pairs[i].identifier != frame_settings.pairs[i].identifier) {
            printf("ERROR: Identifier in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings->pairs[i].identifier);
            printf("wanted %d.\n", frame_settings.pairs[i].identifier);
            return -1;
            //return false;
        }
        if (decoded_settings->pairs[i].value != frame_settings.pairs[i].value) {
            printf("ERROR: Value in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings->pairs[i].value);
            printf("wanted %d.\n", frame_settings.pairs[i].value);
            return - 1;
            //return false;
        }
    }
    printf("settingEncodeDecodeTest OK\n");
    return 0;
}
/*
int main(void) {
    
    frameHeaderEncodeDecodeTest();
    settingEncodeDecodeTest();
    
    return 0;
}
*/


int checkEqualSettingsFrame(settingsframe_t* s1, settingsframe_t* s2, uint32_t size){
    for(uint32_t i = 0; i < size/6; i++) {
        if (s1->pairs[i].identifier != s2->pairs[i].identifier) {
            printf("ERROR: Identifier in settings %d don't match\n", i);
            printf("found %d, ", s1->pairs[i].identifier);
            printf("wanted %d.\n", s2->pairs[i].identifier);
            return -1;
            //return false;
        }
        if (s1->pairs[i].value != s2->pairs[i].value) {
            printf("ERROR: Value in settings %d don't match\n", i);
            printf("found %d, ", s1->pairs[i].value);
            printf("wanted %d.\n", s2->pairs[i].value);
            return - 1;
            //return false;
        }
    };
    return 0;

}

int frameEncodeDecodeTest(int argc, char **argv){
    if ((argc-1)%2!=0){
        printf("usage: %s <id_1> <value_1> ... <id_i> <value_i>\n", argv[0]);
        return 1;
    }
    printf("frameEncodeDecodeTest\n");


    int count = (argc-1)/2;
    uint16_t* ids = (uint16_t*)malloc(sizeof(uint16_t)*count);
    uint32_t* values = (uint32_t*)malloc(sizeof(uint32_t)*count);
    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)atoi(argv[2*i+1]);
        values[i] = (uint32_t)atoi(argv[2*i+1+1]);
    }
    printf("\n");

    frame_t* frame = createSettingsFrame(ids, values, count);
    uint8_t* bytes = (uint8_t*)malloc(frame->frame_header->length+9);
    int size = frameToBytes(frame, bytes);

    frame_t* decoded_frame = bytesToFrame(bytes, size);

    /*Header frame checking*/
    if(frame->frame_header->length != decoded_frame->frame_header->length){
        printf("ERROR: length don't match\n");
        return -1;
    }
    if(frame->frame_header->flags != decoded_frame->frame_header->flags){
        printf("ERROR: flags don't match\n");
        return -1;
    }
    if(frame->frame_header->type != decoded_frame->frame_header->type){
        printf("ERROR: type don't match\n");
        return -1;
    }
    if(frame->frame_header->stream_id != decoded_frame->frame_header->stream_id){
        printf("ERROR: stream_id don't match\n");
        return -1;
    }
    if(frame->frame_header->reserved != decoded_frame->frame_header->reserved){
        printf("ERROR: reserved don't match\n");
        return -1;
    }

    /*payload checking*/
    switch(decoded_frame->frame_header->type){
        case 0x0:{
            printf("Error: not implemented yet");
            return -1;

        }
        case 0x4: {
            int check = checkEqualSettingsFrame((settingsframe_t *) (decoded_frame->payload),
                                                (settingsframe_t *) (frame->payload), frame->frame_header->length);
            if(check == -1){
                return -1;
            }
            break;
        }
        default: {
            printf("Error: not implemented yet");
            return -1;
        }

    }
    printf("frameEncodeDecodeTest OK");
    return 0;


}
