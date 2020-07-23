#ifndef FIRMWARE_EVENTHANDLER_H
#define FIRMWARE_EVENTHANDLER_H

#include <map>
#include "MonitorTemperature.h"

extern MonitorTemperature monitor;

void acNotification(bool power, uint8_t mode, uint8_t temperature);

void autoAcOn(void);
void autoAcOff(void);



void autoAcOn(void) ;

void autoAcOff(void) ;

extern std::map<String, void (*)(void)> event_functions;

#endif //FIRMWARE_EVENTHANDLER_H
