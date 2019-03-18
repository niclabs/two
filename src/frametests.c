
//#include "frames.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> 
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
    int reserved = (bool)atoi(argv[4]);
    int stream_id = (uint32_t)atoi(argv[5]);


    printf("TEST: frameHeaderEncodeDecodeTest\n");
    frameheader_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t* frame_bytes = frameHeaderToBytes(&frame_header);
    frameheader_t* decoder_frame_header = bytesToFrameHeader(frame_bytes, 9);
    
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


    //settingspair_t setting_1 = {identifier_1, value_1};
    //settingspair_t setting_2 = {identifier_2, value_2};
    //settingspair_t settings[2] = {setting_1, setting_2};
    settingsframe_t frame_settings = {(settingspair_t *) &settings};

    uint8_t *setting_frame_bytes = settingsFrameToBytes(&frame_settings, count/2);
    /*    
    for(int i=0; i < 2*6;){
        
        printf("%u\t", setting_frame_bytes[i]);
        i++;
        if(i%2==0)
            printf("\n");
    }
    printf("\n");
    */

    settingsframe_t *decoded_settings = bytesToSettingsFrame(setting_frame_bytes, 6*count/2);

    for (int i = 0; i < count/2; i++) {
        if (decoded_settings->settings[i].identifier != frame_settings.settings[i].identifier) {
            printf("ERROR: Identifier in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings->settings[i].identifier);
            printf("wanted %d.\n", frame_settings.settings[i].identifier);
            return -1;
            //return false;
        }
        if (decoded_settings->settings[i].value != frame_settings.settings[i].value) {
            printf("ERROR: Value in settings %d don't match\n", i);
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