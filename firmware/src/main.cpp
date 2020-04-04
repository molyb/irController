#include "Arduino.h"

#include <ESP8266WebServer.h>
// https://github.com/markszabo/IRremoteESP8266
#include <IRremoteESP8266.h>
#include "infrared.h"
#include "sensor.h"

#define JST     3600*9

extern const char* ssid;
extern const char* password;

ESP8266WebServer server(80);
MonitorTemperature ac(60);

void handleRoot(void) {
    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/plain", "IR Controller");
}

void handleTemperature(void) {
    ac.update();
    float temperature = ac.getTemperature();

    String message = "temperature: " + (String)temperature + " [deg]";
    server.send(200, "text/plain", message);
}


void handleNotFound(void) {
    String message = "File Not Found\n\n";
    message += "URI: ";
    message += server.uri();
    message += "\nMethod: ";
    message += (server.method() == HTTP_GET) ? "GET" : "POST";
    message += "\nArguments: ";
    message += server.args();
    message += "\n";
    for (uint8_t i = 0; i < server.args(); i++) {
        message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
    }
    // HTTPステータスコード(404) 未検出(存在しないファイルにアクセス)
    server.send(404, "text/plain", message);
}


void setup() {
    Serial.begin(115200);
    delay(10);

    // WIFI_AP, WIFI_STA, WIFI_AP_STA or WIFI_OFF
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);

    Serial.print("\n");
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected");
    Serial.print( "IP address: ");
    Serial.println( WiFi.localIP());
    Serial.print( "Gatewat: ");
    Serial.println(WiFi.gatewayIP());
    Serial.print( "Subnet mask: ");
    Serial.println(WiFi.subnetMask());

    configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp");
    Serial.println("Sync ntp");
    while (time(NULL) == 0) {
        delay(500);
        Serial.print(".");
    }
    Serial.println("");
    Serial.println("finished.");

    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);

    server.on("/temperature", handleTemperature);
    server.on("/light", handleLight);
    server.on("/hitachi-ac", handleHitachiAc);

    server.begin();
    Serial.println("HTTP Server started");

    ac.update();
    Serial.print("temperature: ");
    Serial.print(ac.getTemperature(), 3);
    Serial.println("");
}

void loop() {
    server.handleClient();
//    ac.update();
//    Serial.print("temperature: ");
//    Serial.print(ac.getTemperature(), 3);
//    Serial.println("");
//    delay(1000);
}
