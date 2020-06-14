#include "RtcEvent.h"
#include "Arduino.h"

// struct *tm->tm_year に1以下を入れるとmktime結果がバグるので
// 適当に2000年（1900 + 100）を基準にしておく
#define ESCAPE_BUG_MKTIME_OFFSET 100

// ESP8266 Non-OS SDKのソフトタイマ許容時間（約114分）以上の時間間隔でのコールバックが必要な際、
// コールバックを再設定するための待機時間
#define CALLBACK_RELAY_INTERVAL_SEC (60 * 60)  // 1 hour
// コールバックのインターバル時間とちょうど同じ時間後のイベント要求が来た場合に
// イベント登録処理が入ると再登録されず実行されない可能性があるため中継するかどうかの判断時間にオフセットをつける
#define CALLBACK_RELAY_OFFSET_SEC (5 * 60)  // 5 min

RtcEvent::RtcEvent(void) {
    return;
}


bool RtcEvent::append(uint8_t hour_24, uint8_t minute, std::function<void(void)> callback) {
    if (23 < hour_24 || 59 < minute) {
        return false;
    }
    time_t event_time = 0;
    struct tm *show_tm = localtime(&event_time);
    show_tm->tm_year = ESCAPE_BUG_MKTIME_OFFSET;
    show_tm->tm_mon = 0;
    show_tm->tm_mday = 1;
    show_tm->tm_hour = hour_24;
    show_tm->tm_min = minute;
    event_time = mktime(show_tm);

    auto itr = times_.begin();
    // 時間で降順になるようにitrを移動
    while (itr != times_.end()) {
        if (0 < difftime(itr->first, event_time)) {
            break;
        }
        itr++;
    }
    std::pair<time_t, std::function<void(void)>> event(event_time, callback);
    times_.insert(itr, event);
    return true;
}


bool RtcEvent::ready(void) {
    if (times_.empty() || is_readied_) {
        return false;
    }
    time_t current_time;
    struct tm *current_tm;

    current_time = time(NULL);
    current_tm = localtime(&current_time);
    current_tm->tm_year = ESCAPE_BUG_MKTIME_OFFSET;
    current_tm->tm_mon = 0;
    current_tm->tm_mday = 1;
    current_time = mktime(current_tm);

    Serial.print("current: " + (String) asctime(current_tm));

    auto itr = times_.begin();
    double tick_sec = 0;
    while (itr != times_.end()) {
        tick_sec = difftime(itr->first, current_time);
        if (0 < tick_sec) {
            Serial.print("tick_sec: ");
            Serial.println(tick_sec);
            break;
        }
        itr++;
    }
    // その日のスケジュールが全て終了していた場合
    if (itr == times_.end()) {
        time_t tomorrow_event = times_.begin()->first;
        struct tm *tomorrow_tm = localtime(&tomorrow_event);
        tomorrow_tm->tm_mday += 1;
        tomorrow_event = mktime(tomorrow_tm);
        tick_sec = difftime(tomorrow_event, current_time);
        Serial.print("tomorrow_tick_sec: ");
        Serial.println(tick_sec);
    }

    is_readied_ = true;
    callback_ = itr->second;
    if ((CALLBACK_RELAY_INTERVAL_SEC + CALLBACK_RELAY_OFFSET_SEC) < tick_sec) {
        ticker_.once_scheduled(CALLBACK_RELAY_INTERVAL_SEC, std::bind(&RtcEvent::callbackRelay, this));
    } else {
        ticker_.once_scheduled(tick_sec, std::bind(&RtcEvent::callbackAgent, this));
    }
    return true;
}


// ESP8266 Non-OS SDKのソフトタイマ許容時間（約114分）以上の時間間隔でのコールバックが必要な際、
// コールバックを再設定するための関数
void RtcEvent::callbackRelay(void) {
    is_readied_ = false;
    ready();  // 次回用に再度設定
}


void RtcEvent::callbackAgent(void) {
    is_readied_ = false;
    if (callback_ != NULL) {
        callback_();
        callback_ = NULL;
    }
    ready();  // 次回用に再度設定
}


std::pair<time_t, std::function<void(void)>> RtcEvent::isAppended(uint8_t index) {
    auto itr = times_.begin();
    uint8_t i = index;
    while (0 < i) {
        itr++;
        i--;
    }
    return *itr;
}

