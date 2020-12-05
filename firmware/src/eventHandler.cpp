#include "eventHandler.h"

#include "Arduino.h"
#include "MonitorTemperature.h"
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_hitachi.h>

#include "parameters.h"

//const uint8_t kHitachiAc424Fan = 1;
//const uint8_t kHitachiAc424Cool = 3;
//const uint8_t kHitachiAc424Dry = 5;
//const uint8_t kHitachiAc424Heat = 6;

// 必要に応じてprivate_config.cpp等に定義を記述
void acNotification(bool power, uint8_t mode, uint8_t temperature) __attribute__((weak));
void acNotification(bool power, uint8_t mode, uint8_t temperature) {
    return;
}

static void acOnCool(void) {
    IRHitachiAc424 ac(IR_OUT_PIN);
    uint8_t target_temp = 28;
    ac.begin();
    ac.on();
    ac.setMode(kHitachiAc424Cool);
    ac.setTemp(target_temp);
    ac.setFan(kHitachiAc424FanAuto);
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    Serial.println(ac.toString());
    acNotification(true, kHitachiAc424Cool, target_temp);
}

static void acOnHeat(void) {
    IRHitachiAc424 ac(IR_OUT_PIN);
    uint8_t target_temp = 20;
    ac.begin();
    ac.on();
    ac.setMode(kHitachiAc424Heat);
    ac.setTemp(target_temp);
    ac.setFan(kHitachiAc424FanAuto);
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    Serial.println(ac.toString());
    acNotification(true, kHitachiAc424Heat, target_temp);
}

void autoAcOn(void) {
    float temp = monitor.temperature();
    if (temp <= 17.5) {
        acOnHeat();
    } else if (30. <= temp) {
        acOnCool();
    } else {
        return;
    }
}

void autoAcOff(void) {
    IRHitachiAc424 ac(IR_OUT_PIN);
    ac.begin();
    ac.off();
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    Serial.println(ac.toString());
    acNotification(false, 0, 0);
}

std::map<String, void (*)(void)> event_functions;