#include "MonitorTemperature.h"
#include "Arduino.h"

MonitorTemperature::MonitorTemperature(uint16_t interval_sec)
{
    interval_sec_ = interval_sec;

    temp_sensor_ = Adafruit_ADT7410();
    temp_sensor_.begin();
    update();  // ひとまず初期値を入れておく
    temperature_ = temp_sensor_.readTempC();
    ticker_.attach_scheduled(interval_sec_, std::bind(&MonitorTemperature::update, this));
}


void MonitorTemperature::update(void)
{
    temperature_ = temp_sensor_.readTempC();
}


float MonitorTemperature::temperature(void) {
    return temperature_;
}

