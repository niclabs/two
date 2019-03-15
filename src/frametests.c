
//#include "frames.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> 
#include "frames.h"


/*tests if trnasforming from frameHeader to bytes (and vice versa) works*/
bool frameHeaderEncodeDecodeTest(void){
    printf("TEST: frameHeaderEncodeDecodeTest\n");
    frameheader_t frame_header = {12, 0x4, 0x0, 1, 100};
    uint8_t* frame_bytes = frameHeaderToBytes(&frame_header);
    frameheader_t* decoder_frame_header = bytesToFrameHeader(frame_bytes, 9);
    
    if(frame_header.length!=decoder_frame_header->length){
        printf("ERROR: length don't match\n");
        return false;
    }
    if(frame_header.flags!=decoder_frame_header->flags){
        printf("ERROR: flags don't match\n");
        return false;
    }
    if(frame_header.type!=decoder_frame_header->type){
        printf("ERROR: type don't match\n");
        return false;
    }
    if(frame_header.stream_id!=decoder_frame_header->stream_id){
        printf("ERROR: stream_id don't match\n");
        return false;
    }
    if(frame_header.reserved!=decoder_frame_header->reserved){
        printf("ERROR: reserved don't match\n");
        return false;
    }
    printf("frameHeaderEncodeDecodeTest OK\n");
    return true;
}

/*tests if trnasforming from settingsframe to bytes (and vice versa) works*/
bool settingEncodeDecodeTest(void){
    printf("TEST: settingEncodeDecodeTest\n");
    
    uint16_t identifier_1 = 1;
    uint16_t identifier_2 = 2;
    uint32_t value_1 = 1;
    uint32_t value_2 = 2;
    
    
    settingspair_t setting_1 = {identifier_1, value_1};
    settingspair_t setting_2 = {identifier_2, value_2};
    settingspair_t settings[2] = {setting_1, setting_2};
    settingsframe_t frame_settings = {(settingspair_t*)&settings};
    
    uint8_t* setting_frame_bytes = settingsFrameToBytes(&frame_settings, 2);
    /*    
    for(int i=0; i < 2*6;){
        
        printf("%u\t", setting_frame_bytes[i]);
        i++;
        if(i%2==0)
            printf("\n");
    }
    printf("\n");
    */
    
    settingsframe_t* decoded_settings = bytesToSettingsFrame(setting_frame_bytes, 12);
    
    for(int i = 0; i < 2; i++ ){
        if(decoded_settings->settings[i].identifier!=frame_settings.settings[i].identifier){
            printf("ERROR: Identifier in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings->settings[i].identifier);
            printf("wanted %d.\n", frame_settings.settings[i].identifier);
            //return false;
        }
        if(decoded_settings->settings[i].value!=frame_settings.settings[i].value){
            printf("ERROR: Value in settings %d don't match\n", i);
            //return false;
        }
    }
    printf("settingEncodeDecodeTest OK\n");
    return true;
}

int main(void) {
    
    frameHeaderEncodeDecodeTest();
    settingEncodeDecodeTest();
    
    return 0;
}
