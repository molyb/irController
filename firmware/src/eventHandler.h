#ifndef FIRMWARE_EVENTHANDLER_H
#define FIRMWARE_EVENTHANDLER_H

#include "MonitorTemperature.h"

extern MonitorTemperature monitor;

void acNotification(bool power, uint8_t mode, uint8_t temperature);

void autoAcOn(void);
void autoAcOff(void);

#endif //FIRMWARE_EVENTHANDLER_H
