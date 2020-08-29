#include <Arduino.h>

#include "configMode.h"
#include "operationMode.h"

#define PROG_BUTTON 0


enum Mode {
    kOperationMode, kConfigMode
};

enum Mode mode = kOperationMode;

void setup() {
    pinMode(PROG_BUTTON, INPUT);
    // リセット解除後 1secの時点でボタンが押下されていればWifiの設定モードに入る
    delay(1000);
    if (digitalRead(PROG_BUTTON) == LOW) {
        mode = kConfigMode;
        setupConfigMode();
    } else {
        setupOperationMode();
    }
}

void loop() {
    if (mode == kConfigMode) {
        loopConfigMode();
    } else {
        loopOperationMode();
    }
}
