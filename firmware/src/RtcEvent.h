#ifndef FIRMWARE_RTCEVENT_H
#define FIRMWARE_RTCEVENT_H

#include <stdint.h>
#include <Time.h>
#include <array>
#include <list>
#include <functional>
#include <Ticker.h>

// time_tでは月～金に実行するといった管理ができないため独自型を用意する
typedef struct RtcEventData_ {
    uint8_t hour_24;
    uint8_t minute;
    std::function<void(void)> func;
} RtcEventData;


class RtcEvent {
public:
    RtcEvent(void);
    void clear(void);
    bool append(uint8_t hour_24, uint8_t minute, std::array<bool, 7> weekday,
            std::function<void(void)> callback);
    bool ready(void);
private:
    std::array<std::list<RtcEventData>, 7> times_;  // 1 week = 7 days
    Ticker ticker_;
    void callbackRelay(void);
    void callbackAgent(void);
    std::function<void(void)> callback_;
};


#endif //FIRMWARE_RTCEVENT_H
