//
// Created by newrs on 2020/06/15.
//

#ifndef FIRMWARE_SAVEEVENT_H
#define FIRMWARE_SAVEEVENT_H

#include <Arduino.h>
#include <EEPROM.h>


#define SAVE_EVENT_NUMBER_OF_DATA_ADDRESS 0x00  // 4 byte
#define SAVE_EVENT_CHECKSUM_ADDRESS       0x04  // 4 byte
#define SAVE_EVENT_EVENT_BASE_ADDRESS     0x10
#define SAVE_EVENT_FUNC_NAME_LEN          20

struct Event {
    uint8_t hour;
    uint8_t minute;
    char func_name[SAVE_EVENT_FUNC_NAME_LEN];
    void (*func)(void);
};

class SaveEvent {
public:
    SaveEvent(EEPROMClass &eeprom);
    bool checksumIsValid(void);
    void saveChecksum(void);
    void eraseAll(void);
    void erase(uint16_t index);
    bool push(String func_name, void (*func)(void), uint8_t hour, uint8_t minute);

private:
    uint32_t calcChecksum(void);
    void initEvent(Event &event);
    void save(uint16_t index, String func_name, void (*func)(void), uint8_t hour, uint8_t minute);
    EEPROMClass *eeprom_;
    uint16_t save_event_max_;
};

#endif //FIRMWARE_SAVEEVENT_H
