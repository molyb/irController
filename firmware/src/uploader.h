#ifndef FIRMWARE_UPLOADER_H
#define FIRMWARE_UPLOADER_H

#include <ESP8266WiFi.h>
#include "sensor.h"
#include "ambient.h"


class Uploader {
public:
    Uploader(MonitorTemperature* monitor, WiFiClient* client);
    bool enable(unsigned int channel_id, const char * write_key, uint16_t interval_sec);
private:
    void exec(void);
    MonitorTemperature* monitor_;
    WiFiClient* client_;
    Ambient ambient_;
    Ticker ticker_;
};


#endif //FIRMWARE_UPLOADER_H
