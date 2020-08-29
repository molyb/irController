#include "configMode.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>


static const byte DNS_PORT = 53;
static IPAddress apIP(192, 168, 1, 1);
static DNSServer dnsServer;
static ESP8266WebServer config_server(80);


void handleNetworkConfig() {

}


void setupConfigMode(void) {
    Serial.begin(115200);
    Serial.println("\nwakeup in config mode.");
    SPIFFS.begin();

    WiFi.mode(WIFI_AP);
    WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0));
    WiFi.softAP("IR_Controller_AP");

    dnsServer.start(DNS_PORT, "*", apIP);

    config_server.onNotFound([&]() {
        config_server.sendHeader("Location", String("http://") + config_server.client().localIP().toString(), true);
        config_server.send(302, "text/plain", "");
        config_server.client().stop();
    });

    config_server.on("/", [&]() {
        String path = "/networkConfig.html";
        if (SPIFFS.exists(path)) {
            File file = SPIFFS.open(path, "r");
            config_server.streamFile(file, "text/html");
        } else {
            config_server.send(404, "text/plain", "File Not Found!!\n");
        }
    });

    config_server.on("/network-config", handleNetworkConfig);

    config_server.begin();
}


void loopConfigMode(void) {
    config_server.handleClient();
    dnsServer.processNextRequest();
}