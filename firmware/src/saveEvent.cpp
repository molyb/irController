//
// Created by newrs on 2020/06/15.
//

#include "saveEvent.h"

#define SAVE_EVENT_MEM_SIZE 1024

SaveEvent::SaveEvent(EEPROMClass &eeprom) {
    *eeprom_ = eeprom;
    if (eeprom.length() == 0) {
        eeprom_->begin(SAVE_EVENT_MEM_SIZE);
    }
    save_event_max_ = (size_t)eeprom_->length() / (size_t)sizeof(Event);
};


bool SaveEvent::checksumIsValid(void) {
    uint32_t checksum;
    eeprom_->get<uint32_t>(SAVE_EVENT_CHECKSUM_ADDRESS, checksum);
    return checksum == calcChecksum();
}


void SaveEvent::saveChecksum(void) {
    eeprom_->put<uint32_t>(SAVE_EVENT_CHECKSUM_ADDRESS, calcChecksum());
    eeprom_->commit();
}


void SaveEvent::eraseAll(void) {
    for (int i = 0; i < SAVE_EVENT_MEM_SIZE; i++) {
        eeprom_->write(i, 0);
    }
    eeprom_->commit();
}


void SaveEvent::erase(uint16_t index) {
    Event event;
    initEvent(event);
    // put関数内に範囲チェックあるのでそちらに任せる
    eeprom_->put<Event>(SAVE_EVENT_EVENT_BASE_ADDRESS + sizeof(Event) * index, event);
    eeprom_->commit();
}


bool SaveEvent::push(String func_name, void (*func)(void), uint8_t hour, uint8_t minute) {
    for (uint16_t i = 0; i < save_event_max_; i++) {
        Event event;
        eeprom_->get<Event>(SAVE_EVENT_EVENT_BASE_ADDRESS + sizeof(Event) * i, event);
        if (event.func == NULL) {
            save(i, func_name, func, hour, minute);
            return true;
        }
    }
    // ここに到達するという事はメモリに空きがない
    return false;
}


uint32_t SaveEvent::calcChecksum(void) {
    uintptr_t address = SAVE_EVENT_EVENT_BASE_ADDRESS;
    uint32_t checksum = 0x00000000;
    while (address < SAVE_EVENT_MEM_SIZE) {
        checksum += (uint32_t)eeprom_->read(address);
    }
    return checksum;
}


void SaveEvent::initEvent(Event &event) {
    event.hour = 0;
    event.minute = 0;
    for (size_t i = 0; i < SAVE_EVENT_FUNC_NAME_LEN; i++) {
        event.func_name[i] = '\0';
    }
    event.func = NULL;
}


void SaveEvent::save(uint16_t index, String func_name, void (*func)(void), uint8_t hour, uint8_t minute) {
    Event event;
    initEvent(event);
    event.hour = hour;
    event.minute = minute;
    // コピー範囲がMAX-2になっているのはインデックス値調整と最後の1バイトを\0にするため
    for (size_t i = 0; i < func_name.length() && i < SAVE_EVENT_FUNC_NAME_LEN - 2; i++) {
        event.func_name[i] = func_name.charAt(i);
    }
    event.func = func;
    // put関数内に範囲チェックあるのでそちらに任せる
    eeprom_->put<Event>(SAVE_EVENT_EVENT_BASE_ADDRESS + sizeof(Event) * index, event);
    // eepromへのコミット処理はsaveChecksum内にあるので任せる
    saveChecksum();
}
