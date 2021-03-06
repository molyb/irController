#ifndef FIRMWARE_SERVERHANDLER_H
#define FIRMWARE_SERVERHANDLER_H

#include <ESP8266WebServer.h>
#include "MonitorTemperature.h"

extern ESP8266WebServer server;
extern MonitorTemperature monitor;

void handleRoot(void);
void handleTemperature(void);
void handleNotFound(void);
void handleLight(void);
void handleHitachiAc(void);
void handleConfig(void);

#endif //FIRMWARE_SERVERHANDLER_H
