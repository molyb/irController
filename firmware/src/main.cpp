#include "Arduino.h"

#include <ESP8266WebServer.h>
// https://github.com/markszabo/IRremoteESP8266
#include <IRremoteESP8266.h>
#include "infrared.h"
#include "sensor.h"
#include "uploader.h"
#include "RtcEvent.h"
#define JST     3600*9

extern const char* ssid;
extern const char* password;
extern unsigned int ambient_channel_id;
extern const char* ambient_write_key;


ESP8266WebServer server(80);
WiFiClient client;
MonitorTemperature monitor(60);
Uploader uploader(&monitor, &client);
RtcEvent rtc;


void handleRoot(void) {
    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/plain", "IR Controller");
}

void handleTemperature(void) {
    monitor.update();
    float temperature = monitor.getTemperature();

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

void dummy_on(void) {
    Serial.println("dummy_on is called.");
}

void dummy_off(void) {
    Serial.println("dummy_off is called.");
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

//    configTime(0, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp", "time.cloudflare.com");
    configTime(JST, 0, "ntp.nict.jp", "ntp.jst.mfeed.ad.jp", "time.cloudflare.com");
    Serial.print("Sync ntp");
    time_t current_time;
    struct tm* current_tm;
    do  {
        Serial.print(".");
        delay(1000);
        current_time = time(NULL);
        current_tm = localtime(&current_time);
    } while (current_tm->tm_year + 1900 < 2000);
    Serial.println("");
    Serial.print("time info: " + (String)asctime(current_tm));

    Serial.print("current time: ");
    Serial.print(current_tm->tm_year);
    Serial.print("/");
    Serial.print(current_tm->tm_mon);
    Serial.print("/");
    Serial.print(current_tm->tm_mday);
    Serial.print(", ");
    Serial.print(current_tm->tm_hour);
    Serial.print(":");
    Serial.println(current_tm->tm_min);


    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);

    server.on("/temperature", handleTemperature);
    server.on("/light", handleLight);
    server.on("/hitachi-ac", handleHitachiAc);

    server.begin();
    Serial.println("HTTP Server started");

    monitor.update();
    Serial.print("temperature: ");
    Serial.print(monitor.getTemperature(), 3);
    Serial.println("");

    uploader.enable(ambient_channel_id, ambient_write_key, 60);

    rtc.append(8, 00, dummy_on);
    rtc.append(8, 30, dummy_on);
    rtc.append(8, 30, dummy_on);
    rtc.append(7, 30, dummy_on);
    rtc.append(20, 06, dummy_on);
    rtc.append(19, 41, dummy_off);
    rtc.append(0, 41, dummy_off);
    rtc.ready();
}

void loop() {
    server.handleClient();
}
