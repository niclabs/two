#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h> 
#include "utils.h"


/*note the difference between 
frameheader (data that identifies a frame of any type) and 
headersframe(type of frame that contains http headers)*/

typedef struct FrameHeader{
    uint32_t length:24;
    uint8_t type;
    uint8_t flags;
    bool reserved;
    uint32_t stream_id:31;
}frameheader_t; //72 bits-> 9 bytes

typedef struct SettingsPair{
    uint16_t identifier;
    uint32_t value;
}settingspair_t; //48 bits -> 6 bytes

typedef struct SettingsFrame{
    settingspair_t* settings; 
}settingsframe_t; //32 bits -> 4 bytes

typedef struct HeadersFrame{
    uint8_t pad_length; // only if padded flag is set
    bool exclusive_dependency; // only if priority flag is set
    uint32_t stream_dependency:31; // only if priority flag is set
    uint8_t wheight; // only if priority flag is set
    void* header_block_fragment; // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    void* padding; //only if padded flag is set. Size = pad_length
}headersframe_t; //48+32+32 bits -> 14 bytes

typedef struct Frame{
frameheader_t* frame_header;
void * payload;
}frame_t;


uint8_t* frameHeaderToBytes(frameheader_t* frame_header);
frameheader_t* bytesToFrameHeader(uint8_t* bytes, uint8_t length);
//bool frameHeaderEncodeDecodeTest();
uint8_t* settingToBytes(settingspair_t* setting);
uint8_t* settingsFrameToBytes(settingsframe_t* settings_frame, uint8_t count);
settingsframe_t* bytesToSettingsFrame(uint8_t* bytes, uint8_t length);
//bool settingEncodeDecodeTest();