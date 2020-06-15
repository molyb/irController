#ifndef FIRMWARE_RTCEVENT_H
#define FIRMWARE_RTCEVENT_H

#include <stdint.h>
#include <Time.h>
#include <list>
#include <functional>
#include <Ticker.h>

class RtcEvent {
public:
    RtcEvent(void);
    bool append(uint8_t hour_24, uint8_t minute, std::function<void(void)> callback);
    std::pair<time_t, std::function<void(void)>> isAppended(uint8_t index);
    bool ready(void);
private:
    std::list<std::pair<time_t, std::function<void(void)>>> times_;
    Ticker ticker_;
    void callbackRelay(void);
    void callbackAgent(void);
    std::function<void(void)> callback_;
    bool is_readied_;
};


#endif //FIRMWARE_RTCEVENT_H
