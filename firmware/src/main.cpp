#include "Arduino.h"
#include <FS.h>
#include <EEPROM.h>

#include "serverHandler.h"
#include "eventHandler.h"
#include "Uploader.h"
#include "RtcEvent.h"
#include "saveEvent.h"

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
    SPIFFS.begin();

    server.on("/", handleRoot);
    server.onNotFound(handleNotFound);
    server.on("/temperature", handleTemperature);
    server.on("/light", handleLight);
    server.on("/hitachi-ac", handleHitachiAc);
    server.on("/config", handleConfig);
    server.begin();
    Serial.println("HTTP Server started");

    monitor.update();
    Serial.print("temperature: ");
    Serial.print(monitor.temperature(), 3);
    Serial.println("");

    event_functions.insert(std::make_pair("autoAcOn", autoAcOn));
    event_functions.insert(std::make_pair("autoAcOff", autoAcOff));

//    uploader.enable(ambient_channel_id, ambient_write_key, AMBIENT_UPDATE_INTERVAL_SEC);
    EEPROM.begin(1024);
    SaveEvent events(&EEPROM);

    if (!events.checksumIsValid()) {
        events.eraseAll();
        Serial.println("eeprom is erased all.");
        Serial.println("register default events.");
        bool weekday[NUMBER_OF_WEEKDAY] = {false, true, true, true, true, true, false};
        events.push("autoAcOn", event_functions["autoAcOn"], 6, 30, weekday);
        events.push("autoAcOff", event_functions["autoAcOff"], 8, 00, weekday);
        events.push("autoAcOn", event_functions["autoAcOn"], 18, 30, weekday);
    }
    std::list<Event> event_list = events.get();
    for_each (event_list.begin(), event_list.end(), [](Event event) {
        if (event.func != NULL) {
            std::array<bool, 7> weekday = {
                    event.weekday[0], event.weekday[1], event.weekday[2], event.weekday[3],
                    event.weekday[4], event.weekday[5], event.weekday[6]};
            rtc.append(event.hour, event.minute, weekday, event.func);
        }
    });
    rtc.ready();
}

void loop() {
    server.handleClient();
}
