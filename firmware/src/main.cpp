#include "Arduino.h"

#include "parameters.h"

#include "server_handler.h"
// https://github.com/markszabo/IRremoteESP8266
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <ir_hitachi.h>
#include "MonitorTemperature.h"
#include "Uploader.h"
#include "RtcEvent.h"


#define JST (3600 * 9)
#define AMBIENT_UPDATE_INTERVAL_SEC (5 * 60)

extern const char* ssid;
extern const char* password;
extern unsigned int ambient_channel_id;
extern const char* ambient_write_key;

WiFiClient client;
ESP8266WebServer server(80);
MonitorTemperature monitor(60);
Uploader uploader(&monitor, &client);
RtcEvent rtc;

void autoAcOn(void) {
    Serial.println("autoAcOn is called.");
    float temp = monitor.temperature();
    // 現時点では冷房のみ対応
    if (temp < 30.) {
        return;
    }
    IRHitachiAc424 ac(IR_OUT_PIN);
    ac.begin();
    ac.on();
    ac.setMode(kHitachiAc424Cool);
    ac.setTemp(28);
    ac.setFan(kHitachiAc424FanAuto);
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    Serial.println(ac.toString());
}

void autoAcOff(void) {
    IRHitachiAc424 ac(IR_OUT_PIN);
    ac.begin();
    ac.off();
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    Serial.println(ac.toString());
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
    Serial.print((String)asctime(current_tm));

    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.on("/temperature", handleTemperature);
    server.on("/light", handleLight);
    server.on("/hitachi-ac", handleHitachiAc);
    server.begin();
    Serial.println("HTTP Server started");

    monitor.update();
    Serial.print("temperature: ");
    Serial.print(monitor.temperature(), 3);
    Serial.println("");

    uploader.enable(ambient_channel_id, ambient_write_key, AMBIENT_UPDATE_INTERVAL_SEC);

    rtc.append(6, 30, autoAcOn);
    rtc.append(8, 00, autoAcOff);
    rtc.append(18, 30, autoAcOn);
    rtc.ready();
}

void loop() {
    server.handleClient();
}