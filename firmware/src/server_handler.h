//
// Created by newrs on 2020/05/24.
//

#ifndef FIRMWARE_SERVER_HANDLER_H
#define FIRMWARE_SERVER_HANDLER_H

#include <ESP8266WebServer.h>
#include "MonitorTemperature.h"

extern ESP8266WebServer server;
extern MonitorTemperature monitor;

void handleRoot(void);
void handleTemperature(void);
void handleNotFound(void);
void handleLight(void);
void handleHitachiAc(void);

#endif //FIRMWARE_SERVER_HANDLER_H