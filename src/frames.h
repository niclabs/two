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

/*FRAME TYPES*/
typedef enum FrameType{
    DATA_TYPE = (uint8_t)0x0,
    HEADERS_TYPE = (uint8_t)0x1,
    PRIORITY_TYPE= (uint8_t)0x2,
    RST_STREAM_TYPE = (uint8_t)0x3,
    SETTINGS_TYPE = (uint8_t)0x4,
    PUSH_PROMISE_TYPE = (uint8_t)0x5,
    PING_TYPE= (uint8_t)0x6,
    GOAWAY_TYPE = (uint8_t)0x7,
    WINDOW_UPDATE_TYPE = (uint8_t)0x8,
    CONTINUATION_TYPE = (uint8_t)0x9
}frame_type_t;

/*FRAME HEADER*/
typedef struct FrameHeader{
    uint32_t length:24;
    frame_type_t type;
    uint8_t flags;
    uint8_t reserved:1;
    uint32_t stream_id:31;
}frameheader_t; //72 bits-> 9 bytes

/*FRAME*/
typedef struct Frame{
frameheader_t* frame_header;
void * payload;
}frame_t;


/*SETTINGS FRAME*/
typedef struct SettingsPair{
    uint16_t identifier;
    uint32_t value;
}settingspair_t; //48 bits -> 6 bytes

typedef struct SettingsPayload{
    settingspair_t* pairs;
    int count;
}settingspayload_t; //32 bits -> 4 bytes

typedef enum SettingFlag{
    SETTINGS_ACK_FLAG = 0x1
}setting_flag_t;



/*HEADERS FRAME*/

typedef struct HeadersPayload{
    uint8_t pad_length; // only if padded flag is set
    uint8_t exclusive_dependency:1; // only if priority flag is set
    uint32_t stream_dependency:31; // only if priority flag is set
    uint8_t wheight; // only if priority flag is set
    void* header_block_fragment; // only if length > 0. Size = frame size - (4+1)[if priority is set]-(4+pad_length)[if padded flag is set]
    void* padding; //only if padded flag is set. Size = pad_length
}headerspayload_t; //48+32+32 bits -> 14 bytes

typedef enum HeaderFlag{
    HEADERS_END_STREAM_FLAG = 0x1,//bit 0
    HEADERS_END_HEADERS_FLAG = 0x4,//bit bit 2
    HEADERS_PADDED_FLAG = 0x8,//bit 3
    HEADERS_PRIORITY_FLAG = 0x20//bit 5
}header_flag_t;



/*frame header methods*/
int frameHeaderToBytes(frameheader_t* frame_header, uint8_t* byte_array);
int bytesToFrameHeader(uint8_t* byte_array, int size, frameheader_t* frame_header);

/*flags methods*/
int isFlagSet(uint8_t flags, uint8_t flag);
uint8_t setFlag(uint8_t flags, uint8_t flag_to_set);

/*frame methods*/
int frameToBytes(frame_t* frame, uint8_t* bytes);
//int bytesToFrame(uint8_t * bytes, int size, frame_t* frame);

/*settings methods*/
int createListOfSettingsPair(uint16_t* ids, uint32_t* values, int count, settingspair_t* pair_list);
int createSettingsFrame(uint16_t* ids, uint32_t* values, int count, frame_t* frame, frameheader_t* frame_header, settingspayload_t* settings_frame, settingspair_t* pairs);
int settingToBytes(settingspair_t* setting, uint8_t* byte_array);
int settingsFrameToBytes(settingspayload_t* settings_frame, uint32_t count, uint8_t* byte_array);

int bytesToSettingsPayload(uint8_t* bytes, int size, settingspayload_t* settings_frame, settingspair_t* pairs);

int createSettingsAckFrame(frame_t * frame, frameheader_t* frame_header);


/*
void* byteToPayloadDispatcher[10];
byteToPayloadDispatcher[SETTINGS_TYPE] = &bytesToSettingsPayload;


typedef* payloadTypeDispatcher[10];
payloadTypeDispatcher[SETTINGS_TYPE] = &settingspayload_t
*/
