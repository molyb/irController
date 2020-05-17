#ifndef FIRMWARE_CTRL_SENSOR_H
#define FIRMWARE_CTRL_SENSOR_H

#include <Time.h>
#include "Ticker.h"
#include "Adafruit_ADT7410.h"


class MonitorTemperature
{
public:
    MonitorTemperature(uint16_t monitoring_interval_minute);
    void update(void);
    float getTemperature(void);
private:
    uint16_t monitoring_interval_minute_;
    time_t prev_time_;
    time_t current_time_;
    Ticker ticker_;
    Adafruit_ADT7410 temp_sensor_;
    float temperature_;
};

#endif //FIRMWARE_CTRL_SENSOR_H
