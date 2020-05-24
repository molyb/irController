#ifndef FIRMWARE_CTRL_SENSOR_H
#define FIRMWARE_CTRL_SENSOR_H

#include <Time.h>
#include "Ticker.h"
#include "Adafruit_ADT7410.h"

class MonitorTemperature
{
public:
    MonitorTemperature(uint16_t interval_sec);
    void update(void);
    float temperature(void);
private:
    uint16_t interval_sec_;
    Ticker ticker_;
    Adafruit_ADT7410 temp_sensor_;
    float temperature_;
};

#endif //FIRMWARE_CTRL_SENSOR_H
