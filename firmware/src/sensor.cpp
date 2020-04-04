#include "sensor.h"
#include "Arduino.h"

bool updateIsRequired(void)
{
//    static const char *wd[7] = {"Sun","Mon","Tue","Wed","Thr","Fri","Sat"};
    static time_t prev_time;
    time_t current_time;
    struct tm *prev_tm, *current_tm;

    current_time = time(NULL);
    current_tm = localtime(&current_time);
    prev_tm = localtime(&prev_time);

    bool required = false;
    if (current_tm->tm_hour == 5 || prev_tm->tm_hour != 5) {
        required = true;
    }
    prev_time = current_time;
    return required;
}


MonitorTemperature::MonitorTemperature(uint16_t monitoring_interval_minute)
{
    monitoring_interval_minute_ = monitoring_interval_minute;
    prev_time_ = 0;
    current_time_ = 0;

    temp_sensor_ = Adafruit_ADT7410();
    temp_sensor_.begin();
    update();  // ひとまず初期値を入れておく
    temperature_ = temp_sensor_.readTempC();
    ticker_.attach_scheduled(5., std::bind(&MonitorTemperature::update, this));
}


void MonitorTemperature::update(void)
{
    temperature_ = temp_sensor_.readTempC();
    Serial.print("temperature: ");
    Serial.print(temperature_, 3);
    Serial.println("");
}


float MonitorTemperature::getTemperature(void) {
    return temperature_;
}

