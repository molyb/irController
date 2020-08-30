#include "configMode.h"

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <DNSServer.h>
#include <ESP8266WebServer.h>

#include "saveNetworkConfig.h"

static const byte DNS_PORT = 53;
static IPAddress *ap_ip;
static DNSServer *dnsServer;
static ESP8266WebServer *config_server;


void handleNetworkConfig() {
    if (config_server->method() != HTTP_POST) {
        config_server->send(400, "text/plain", "fail\n");
        return;
    }
    if (!config_server->hasArg("ssid") || !config_server->hasArg("password")) {
        config_server->send(400, "text/plain", "fail\n");
        return;
    }

    SaveNetworkConfig network(&EEPROM);
    network.setSsid(config_server->arg("ssid"));
    network.setPassword(config_server->arg("password"));
    config_server->send(200, "text/plain", "success\n");
}


void setupConfigMode(void) {
    Serial.begin(115200);
    Serial.println("\nwakeup in config mode.");
    SPIFFS.begin();
    EEPROM.begin(1024);

    WiFi.mode(WIFI_AP);
    ap_ip = new IPAddress(192, 168, 1, 1);
    WiFi.softAPConfig(*ap_ip, *ap_ip, IPAddress(255, 255, 255, 0));
    WiFi.softAP("IR_Controller_AP");

    dnsServer = new DNSServer ;
    dnsServer->start(DNS_PORT, "*", *ap_ip);

    config_server = new ESP8266WebServer(80);
    config_server->onNotFound([&]() {
        config_server->sendHeader("Location", String("http://") + config_server->client().localIP().toString(), true);
        config_server->send(302, "text/plain", "");
        config_server->client().stop();
    });

    config_server->on("/", [&]() {
        String path = "/networkConfig.html";
        if (SPIFFS.exists(path)) {
            File file = SPIFFS.open(path, "r");
            config_server->streamFile(file, "text/html");
        } else {
            config_server->send(404, "text/plain", "File Not Found!!\n");
        }
    });

    config_server->on("/network-config", handleNetworkConfig);

    config_server->begin();
}


void loopConfigMode(void) {
    config_server->handleClient();
    dnsServer->processNextRequest();
}