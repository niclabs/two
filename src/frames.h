#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "utils.h"


/*note the difference between 
frameheader (data that identifies a frame of any type) and 
headersframe(type of frame that contains http headers)*/

/*typedef struct ByteArray{
    uint8_t* bytes;
    uint32_t length;
}bytearray_t;
*/
typedef struct FrameHeader{
    uint32_t length:24;
    uint8_t type;
    uint8_t flags;
    uint8_t reserved:1;
    uint32_t stream_id:31;
}frameheader_t; //72 bits-> 9 bytes

typedef struct SettingsPair{
    uint16_t identifier;
    uint32_t value;
}settingspair_t; //48 bits -> 6 bytes

typedef struct SettingsFrame{
    settingspair_t* pairs;
    int count;
}settingsframe_t; //32 bits -> 4 bytes

typedef struct HeadersFrame{
    uint8_t pad_length; // only if padded flag is set
    uint8_t exclusive_dependency:1; // only if priority flag is set
    uint32_t stream_dependency:31; // only if priority flag is set
    uint8_t wheight; // only if priority flag is set
    void* header_block_fragment; // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    void* padding; //only if padded flag is set. Size = pad_length
}headersframe_t; //48+32+32 bits -> 14 bytes

typedef struct Frame{
frameheader_t* frame_header;
void * payload;
}frame_t;


int frameHeaderToBytes(frameheader_t* frame_header, uint8_t* byte_array);
frameheader_t* bytesToFrameHeader(uint8_t* byte_array, int size);
int settingToBytes(settingspair_t* setting, uint8_t* byte_array);
int settingsFrameToBytes(settingsframe_t* settings_frame, uint32_t count, uint8_t* byte_array);
settingsframe_t* bytesToSettingsFrame(uint8_t* bytes, int size);
int frameToBytes(frame_t* frame, uint8_t* bytes);
frame_t* bytesToFrame(uint8_t * bytes, int size);

int createListOfSettingsPair(uint16_t* ids, uint32_t* values, int count, settingspair_t* pair_list);



frame_t* createSettingsFrame(uint16_t* ids, uint32_t* values, int count);

frame_t* createSettingsAckFrame(void);