#include "RtcEvent.h"
#include "Arduino.h"
#include <algorithm>

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


void RtcEvent::clear(void) {
    for (std::list<RtcEventData> &times : times_) {
        times.clear();
    }
}


bool RtcEvent::append(uint8_t hour_24, uint8_t minute, std::array<bool, 7> weekday,
        std::function<void(void)> callback) {
    if (23 < hour_24 || 59 < minute) {
        return false;
    }

    RtcEventData event;
    event.hour_24 = hour_24;
    event.minute = minute;
    event.func = callback;

    uint16_t new_time = event.hour_24 * 60 + event.minute;
    for (uint8_t weekday_index = 0; weekday_index < (uint8_t)times_.size(); weekday_index++) {
        if (!weekday[weekday_index]) {
            continue;
        }
        std::list<RtcEventData> &event_of_weekday = times_.at(weekday_index);
        auto time_itr = event_of_weekday.begin();
        // 時間で降順になるようにitrを移動
        while (time_itr != event_of_weekday.end()) {
            uint16_t target_time = time_itr->hour_24 * 60 + time_itr->minute;
            if (new_time < target_time) {
                break;
            }
            time_itr++;
        }
        event_of_weekday.insert(time_itr, event);
    }
    return true;
}


bool RtcEvent::ready(void) {
    // イベントの削除処理後にも本関数が呼ばれるためtimes_が空かに依らずでタッチしておく
    ticker_.detach();
    // 全ての週でイベントが空だったらリターン
    bool is_empty = true;
    for (std::list<RtcEventData> events_each_weekday : times_) {
        if (!events_each_weekday.empty()) {
            is_empty = false;
        }
    }
    if (is_empty) {
        // 未発火イベントが残ると困るのでデタッチの後にリターン処理
        return false;
    }

    time_t current_time = time(NULL);;
    struct tm *current_tm = localtime(&current_time);

    double tick_sec = 0;
    uint8_t daily_counter = 0;

    bool is_detected = false;
    std::function<void(void)>  detected_function;
    // 変数iは単純に一週間7日分をループさせるためだけの変数
    // 実際に変数にアクセスする際には変数indexを用いる
    uint8_t index = current_tm->tm_wday;
    for (uint8_t i = 0; i < 7; i++) {
        std::list<RtcEventData> today_event = times_.at(index);
        // 現在、各曜日のイベントを降順にサーチしていくので見つけるのが遅いかもしれないが
        // 家庭内で数人で使用する際に問題になる事はないので安易な実装でいく
        for (const RtcEventData &event : today_event) {
            // この時点では曜日については考慮しない。ループの外でつじつまを合わせる
            double tick_min = (event.hour_24 * 60 + event.minute) - (current_tm->tm_hour * 60 + current_tm->tm_min);
            tick_sec = tick_min * 60;
            // イベントは登録時に時間の降順でソートしているので、翌日以降であれば最初に発見したイベントが対象
            if (0 < daily_counter || 0 < tick_sec) {
                is_detected = true;
                callback_ = event.func;
                break;
            }
        }
        if (is_detected) {
            break;
        }

        // 次の曜日のサーチに入るための準備
        auto incrementWeekdayIndex = [](int current_index) {
            current_index++;
            if (7 <= current_index) {
                current_index = 0;
            }
            return current_index;
        };
        index = incrementWeekdayIndex(index);
        daily_counter++;
    }
    // ここまでtick_secでは日付を考慮していなかったのでここでつじつま合わせ
    tick_sec = ((double)daily_counter * 24. * 60. * 60.) + tick_sec;
    Serial.print("tick_sec: ");
    Serial.println(tick_sec);

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
    ready();  // 次回用に再度設定
}


void RtcEvent::callbackAgent(void) {
    if (callback_ != NULL) {
        callback_();
        callback_ = NULL;
    }
    ready();  // 次回用に再度設定
}

