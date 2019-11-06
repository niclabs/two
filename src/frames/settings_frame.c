//
// Created by gabriel on 04-11-19.
//

#include "settings_frame.h"
#include "common.h"
#include "utils.h"
#include "logging.h"

/*
 * Function: setting_to_bytes
 * Pass a settingPair to bytes
 * Input:  settingPair, pointer to bytes
 * Output: size of written bytes
 */
int setting_to_bytes(settings_pair_t *setting, uint8_t *bytes)
{
    uint8_t identifier_bytes[2];

    uint16_to_byte_array(setting->identifier, identifier_bytes);
    uint8_t value_bytes[4];
    uint32_to_byte_array(setting->value, value_bytes);
    for (int i = 0; i < 2; i++) {
        bytes[i] = identifier_bytes[i];
    }
    for (int i = 0; i < 4; i++) {
        bytes[2 + i] = value_bytes[i];
    }
    return 6;
}

/*
 * Function: settings_frame_to_bytes
 * pass a settings payload to bytes
 * Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
 * Output: size of written bytes
 */
int settings_frame_to_bytes(settings_payload_t *settings_payload, uint32_t count, uint8_t *bytes)
{
    for (uint32_t i = 0; i < count; i++) {
        //printf("%d\n",i);
        uint8_t setting_bytes[6];
        int size = setting_to_bytes(settings_payload->pairs + i, setting_bytes);
        for (int j = 0; j < size; j++) {
            bytes[i * 6 + j] = setting_bytes[j];
        }
    }
    return 6 * count;
}

/*
 * Function: bytes_to_settings_payload
 * pass a settings payload to bytes
 * Input:  settingPayload pointer, amount of settingspair in payload, pointer to bytes
 * Output: size of written bytes
 */
int bytes_to_settings_payload(frame_header_t *frame_header, void *payload, uint8_t *bytes)
{
    settings_payload_t *settings_payload = (settings_payload_t *) payload;
    if (frame_header->length % 6 != 0) {
        ERROR("settings payload wrong size\n");
        return -1;
    }
    int count = frame_header->length / 6;

    for (int i = 0; i < count; i++) {
        settings_payload->pairs[i].identifier = bytes_to_uint16(bytes + (i * 6));
        settings_payload->pairs[i].value = bytes_to_uint32(bytes + (i * 6) + 2);
    }
    settings_payload->count = count;
    return (6 * count);
}


/*
 * Function: create_list_of_settings_pair
 * Create a List Of SettingsPairs
 * Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to settings Pairs
 * Output: size of read setting pairs
 */
int create_list_of_settings_pair(uint16_t *ids, uint32_t *values, int count, settings_pair_t *settings_pairs)
{
    for (int i = 0; i < count; i++) {
        settings_pairs[i].identifier = ids[i];
        settings_pairs[i].value = values[i];
    }
    return count;
}

/*
 * Function: create_settings_frame
 * Create a Frame of type sewttings with its payload
 * Input:  pointer to ids array, pointer to values array, size of those arrays,  pointer to frame, pointer to frameheader, pointer to settings payload, pointer to settings Pairs.
 * Output: 0 if setting frame was created
 */
int create_settings_frame(uint16_t *ids, uint32_t *values, int count, frame_t *frame, frame_header_t *frame_header,
                          settings_payload_t *settings_payload, settings_pair_t *pairs)
{
    frame_header->length = count * 6;
    frame_header->type = SETTINGS_TYPE;//settings;
    frame_header->flags = 0x0;
    frame_header->reserved = 0x0;
    frame_header->stream_id = 0;
    count = create_list_of_settings_pair(ids, values, count, pairs);
    settings_payload->count = count;
    settings_payload->pairs = pairs;
    frame->payload = (void *)settings_payload;
    frame->frame_header = frame_header;
    return 0;
}

