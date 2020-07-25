//
// Created by newrs on 2020/06/15.
//

#ifndef FIRMWARE_SAVEEVENT_H
#define FIRMWARE_SAVEEVENT_H

#include <Arduino.h>
#include <EEPROM.h>
#include <list>

#define SAVE_EVENT_NUMBER_OF_DATA_ADDRESS 0x00  // 4 byte
#define SAVE_EVENT_CHECKSUM_ADDRESS       0x04  // 4 byte
#define SAVE_EVENT_EVENT_BASE_ADDRESS     0x10
#define SAVE_EVENT_FUNC_NAME_LEN          20

#define NUMBER_OF_WEEKDAY 7

struct Event {
    uint8_t hour;  // 0 ... 23
    uint8_t minute; // 0 ... 59
    bool weekday[NUMBER_OF_WEEKDAY]; // sun ... sat
    char func_name[SAVE_EVENT_FUNC_NAME_LEN];
    void (*func)(void);
};

class SaveEvent {
public:
    SaveEvent(EEPROMClass *eeprom);
    bool checksumIsValid(void);
    bool saveChecksum(void);
    void eraseAll(void);
    // 登録済みのイベント数に応じたインデックスでerase
    bool erase(uint16_t registered_event_index);
    bool push(String func_name, void (*func)(void), uint8_t hour, uint8_t minute, bool* weekday);
    std::list<Event> get(void);

private:
    uint32_t calcChecksum(void);
    void initEvent(Event &event);
    // eeprom内部の物理配置に応じてerase
    bool eraseInternalIndex(uint16_t index);
    void save(uint16_t index, String func_name, void (*func)(void), uint8_t hour, uint8_t minute, bool* weekday);
    EEPROMClass *eeprom_;
    uint16_t save_event_max_;
};

#endif //FIRMWARE_SAVEEVENT_H
