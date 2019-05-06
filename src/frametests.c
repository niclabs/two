#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "frames.h"

/*tests if trnasforming from frameHeader to bytes (and vice versa) works*/
int frame_header_encode_decode_test(int argc, char **argv){
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
    frame_header_t frame_header = {length, type, flags, reserved, stream_id};
    uint8_t frame_bytes[9];
    int frame_bytes_size = frame_header_to_bytes(&frame_header, frame_bytes);

    frame_header_t decoder_frame_header;
    bytes_to_frame_header(frame_bytes, frame_bytes_size, &decoder_frame_header);
    
    if(frame_header.length!=decoder_frame_header.length){
        printf("ERROR: length don't match\n");
        return -1;
    }
    if(frame_header.flags!=decoder_frame_header.flags){
        printf("ERROR: flags don't match\n");
        return -1;
    }
    if(frame_header.type!=decoder_frame_header.type){
        printf("ERROR: type don't match\n");
        return -1;
    }
    if(frame_header.stream_id!=decoder_frame_header.stream_id){
        printf("ERROR: stream_id don't match\n");
        return -1;
    }
    if(frame_header.reserved!=decoder_frame_header.reserved){
        printf("ERROR: reserved don't match\n");
        return -1;
    }
    printf("frameHeaderEncodeDecodeTest OK\n");
    return 0;
}

/*tests if trnasforming from settingsframe to bytes (and vice versa) works*/
int setting_encode_decode_test(int argc, char **argv){
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


    settings_pair_t  settings[count/2];
    for (int i = 0; i < count/2; i++){
        settings_pair_t pair = {ids[i],values[i]};
        settings[i] = pair;
    }


    settings_payload_t frame_settings = {(settings_pair_t *) &settings, count/2};

    uint8_t setting_frame_bytes[count/2*6];
    int size = settings_frame_to_bytes(&frame_settings, count/2, setting_frame_bytes);

    settings_payload_t decoded_settings;
    settings_pair_t pairs[count];
    size = bytes_to_settings_payload(setting_frame_bytes, 6*count/2, &decoded_settings, pairs);

    for (int i = 0; i < size/6; i++) {
        if (decoded_settings.pairs[i].identifier != frame_settings.pairs[i].identifier) {
            printf("ERROR: Identifier in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings.pairs[i].identifier);
            printf("wanted %d.\n", frame_settings.pairs[i].identifier);
            return -1;
            //return false;
        }
        if (decoded_settings.pairs[i].value != frame_settings.pairs[i].value) {
            printf("ERROR: Value in settings %d don't match\n", i);
            printf("found %d, ", decoded_settings.pairs[i].value);
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


int check_equal_settings_frame(settings_payload_t *s1, settings_payload_t *s2, uint32_t size){
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

int frame_encode_decode_test(int argc, char **argv){
    if ((argc-1)%2!=0){
        printf("usage: %s <id_1> <value_1> ... <id_i> <value_i>\n", argv[0]);
        return 1;
    }
    printf("frameEncodeDecodeTest\n");


    int count = (argc-1)/2;
    uint16_t ids[count];
    uint32_t values[count];

    for (int i = 0; i < count; i++){
        ids[i] = (uint16_t)atoi(argv[2*i+1]);
        values[i] = (uint32_t)atoi(argv[2*i+1+1]);
    }
    printf("\n");
    frame_t frame;
    frame_header_t frame_header;
    settings_payload_t settings_payload;
    settings_pair_t setting_pairs[count];

    int size;
    size = create_settings_frame(ids, values, count, &frame, &frame_header, &settings_payload, setting_pairs);

    uint8_t bytes[frame.frame_header->length+9];
    size = frame_to_bytes(&frame, bytes);

    frame_header_t decoded_frame_header;
    bytes_to_frame_header(bytes, size, &decoded_frame_header);

    /*Header frame checking*/
    if(frame.frame_header->length != decoded_frame_header.length){
        printf("ERROR: length don't match\n");
        return -1;
    }
    if(frame.frame_header->flags != decoded_frame_header.flags){
        printf("ERROR: flags don't match\n");
        return -1;
    }
    if(frame.frame_header->type != decoded_frame_header.type){
        printf("ERROR: type don't match\n");
        return -1;
    }
    if(frame.frame_header->stream_id != decoded_frame_header.stream_id){
        printf("ERROR: stream_id don't match\n");
        return -1;
    }
    if(frame.frame_header->reserved != decoded_frame_header.reserved){
        printf("ERROR: reserved don't match\n");
        return -1;
    }


    /*payload checking*/
    switch(decoded_frame_header.type){
        case 0x0:{
            printf("Error: not implemented yet 0x0");
            return -1;

        }
        case 0x4: {
            settings_payload_t payload;
            settings_pair_t pairs[decoded_frame_header.length/6];
            bytes_to_settings_payload(bytes+9, decoded_frame_header.length, &payload, pairs);
            int check = check_equal_settings_frame((settings_payload_t *) (&payload),
                                                (settings_payload_t *) (frame.payload), frame.frame_header->length);
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
