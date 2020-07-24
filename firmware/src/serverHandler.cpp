#include "serverHandler.h"
#include "MonitorTemperature.h"
// https://github.com/markszabo/IRremoteESP8266
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <ir_hitachi.h>
#include <ArduinoJson.h>
#include "parameters.h"
#include "saveEvent.h"
#include "eventHandler.h"
#include "RtcEvent.h"

const unsigned int ir_out_pin = IR_OUT_PIN;
extern RtcEvent rtc;

void handleRoot(void) {
    File index = SPIFFS.open("/index.html", "r");
    if(!index) {
        Serial.println("Fail: load index.html");
        handleNotFound();
        return;
    }
    String html = index.readString();
    index.close();
    server.send(200, "text/html", html);
}


void handleNotFound(void) {
    // 内部ファイルシステムにファイルがあればリード
    String path = server.uri();
    if (SPIFFS.exists(path)) {
        File file = SPIFFS.open(path, "r");
        auto convertMimeType = [](String filename) -> String {
            if(filename.endsWith(".htm")) return "text/html";
            else if(filename.endsWith(".html")) return "text/html";
            else if(filename.endsWith(".css")) return "text/css";
            else if(filename.endsWith(".js")) return "application/javascript";
            else if(filename.endsWith(".png")) return "image/png";
            else if(filename.endsWith(".gif")) return "image/gif";
            else if(filename.endsWith(".jpg")) return "image/jpeg";
            else if(filename.endsWith(".ico")) return "image/x-icon";
            else if(filename.endsWith(".xml")) return "text/xml";
            return "text/plain";
        };
        String contentType = convertMimeType(path);
        server.streamFile(file, contentType);
        file.close();
        return;
    }

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


void handleTemperature(void) {
    monitor.update();
    float temperature = monitor.temperature();

    String message = "temperature: " + (String)temperature + " [deg]";
    server.send(200, "text/plain", message);
}


void handleLight(void) {
    File index = SPIFFS.open("/light.html", "r");
    if(!index) {
        Serial.println("Fail: load light.html");
        handleNotFound();
        return;
    }
    String html = index.readString();
    index.close();

    const uint64_t irPatternNecLightOn = 0x41B6659A;
    const uint64_t irPatternNecLightNight = 0x41b63dc2;
    const uint64_t irPatternNecLightOff = 0x41B67D82;
    const uint16_t irPatternBitsLightOn = 32;
    const uint16_t irPatternBitsLightNight = 32;
    const uint16_t irPatternBitsLightOff = 32;

    IRsend irsend(ir_out_pin);
    irsend.begin();
    enum State {
        unknown,
        on,
        night,
        off
    };
    __unused enum State state = unknown;
    //  LED の制御(server.method()でメソッドごとの処理を切り替えられるが今は同じにしておく)
    String val = server.arg("light");
    if (val == "on") {
        Serial.println("debug: on");
        irsend.sendNEC(irPatternNecLightOn, irPatternBitsLightOn);
        state = on;
    } else if (val == "night") {
        Serial.println("debug: night");
        irsend.sendNEC(irPatternNecLightNight, irPatternBitsLightNight);
        state = night;
    } else if (val == "off") {
        Serial.println("debug: off");
        irsend.sendNEC(irPatternNecLightOff, irPatternBitsLightOff);
        state = off;
    } else {
        ;
    }

    __unused auto state2str = [](enum State state) {
        if (state == on) {
            return String("on");
        } else if (state == night) {
            return String("night");
        } else if (state == off) {
            return String("off");
        } else {
            ;
        }
        return String("unknown");
    };

    server.send(200, "text/html", html);
}

void handleHitachiAc(void) {
    IRHitachiAc424 ac(ir_out_pin);
    ac.begin();

    String power = "off";
    String operation_mode = "fan";
    String temp = "22";
    String fan = "auto";
    String swing = "off";

    power = server.arg("power");
    operation_mode = server.arg("mode");
    temp = server.arg("temp");
    fan = server.arg("fan");
    // swingは実装の手間と実用面を考慮してサポートしない
    // IRremoteESP8266ライブラリのIRHitachiAc424メンバ関数setSwingVToggleコメントを参照
    // swing = server.arg("swing");

    if (power == "on") {
        ac.on();
    } else {
        ac.off();
    }
    if (operation_mode == "fan") {
        ac.setMode(kHitachiAc424Fan);
    } else if (operation_mode == "cool") {
        ac.setMode(kHitachiAc424Cool);
    } else if (operation_mode == "heat") {
        ac.setMode(kHitachiAc424Heat);
    } else if (operation_mode == "dry") {
        ac.setMode(kHitachiAc424Dry);
    } else {
        // IRHitachiAc424にはオートが無いのでひとまず送風にしておく
        ac.setMode(kHitachiAc424Fan);
    }
    // 数値に変換できない場合は0が返ってくるのでsetTempで保護処理される
    uint8_t num = strtol(temp.c_str(), NULL, 10);
    ac.setTemp(num);
    if (fan == "low") {
        ac.setFan(kHitachiAc424FanLow);
    } else if (fan == "medium") {
        ac.setFan(kHitachiAc424FanMedium);
    } else if (fan == "high") {
        ac.setFan(kHitachiAc424FanHigh);
    } else {
        // ライブラリにautoはあったが設定するとlowになる
        // 実際にも機能として無さそうなのでelseをlowにしてもいいかも
        ac.setFan(kHitachiAc424FanAuto);
    }
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();

    String header = "\
<html lang=\"ja\">\n\
    <meta charset=\"utf-8\">\n\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\
    <head>\n\
        <title>IrController</title>\n\
    </head>\n";

    String body = "\
<body style=\"font-family: sans-serif; background-color: #ffffff;\" >\n\
    <h1>Hitachi AC Controller</h1>\n\
    <p>\
    <form>\
        <p> Power\
            <select name=\"power\">\
                <option value=\"on\">On</option>\
                <option value=\"off\">Off</option>\
            </select>\
        </p>\
        <p> Mode\
            <select name=\"mode\">\
                <option value=\"fan\">Fan</option>\
                <option value=\"dry\">Dry</option>\
                <option value=\"cool\">Cool</option>\
                <option value=\"heat\">Heat</option>\
            </select>\
        </p>\
        <p> Temperature\
            <input type=\"number\" name=\"temp\" value=\"27\">\
        </p>\
        <p> Fan\
            <select name=\"fan\">\
                <option value=\"low\">Low</option>\
                <option value=\"mid\">Mid</option>\
                <option value=\"high\">High</option>\
            </select>\
        </p>\
        <input type=\"submit\" value=\"Submit\" />\
    </form>\
    </p>\n";

    String state = ac.toString();
    body += "<p>" + state + "</p>\n";
    body += "</body>\n";

    String footer = "</html>\n";
    String message = header + body + footer;

    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/html", message);
}



// 以下のようなjsonファイルを生成する事を想定している
//{
//    "title": "over written title",
//    "events": [
//        {
//            "function_name": "func0",
//            "hour": 6,
//            "minute": 30
//        },
//        {
//            "function_name": "func1",
//            "hour": 8,
//            "minute": 0
//        }
//    ]
//}
void handleConfig(void) {
    SaveEvent events(&EEPROM);
    if (server.method() == HTTP_POST) {
        // 同じname属性で複数の値を投げる実装になっているのでループを回して全部の引数について調査する
        for (int i = 0; i < server.args(); i++) {
            if (server.argName(i) == String("delete_index")) {
                Serial.print("delete_index:");
                Serial.println(server.arg(i));
                events.erase(server.arg(i).toInt());
            } else if (server.argName(i) == String("register_event")) {
                String function_name = server.arg(i);
                String time = server.arg("register_time");
                int hour = time.substring(0, time.indexOf(":")).toInt();
                int minute = time.substring(time.indexOf(":") + 1).toInt();
                events.push(function_name, event_functions[function_name], hour, minute);
            }
        }

        rtc.clear();
        std::list<Event> event_list = events.get();
        for_each (event_list.begin(), event_list.end(), [](Event event) {
            if (event.func != NULL) {
                rtc.append(event.hour, event.minute, event.func);
            }
        });
        rtc.ready();

        String file_path = "/events.html";
        if (SPIFFS.exists(file_path)) {
            File file = SPIFFS.open(file_path, "r");
            server.streamFile(file, "text/html");
            file.close();
            return;
        }
    }

    std::list<Event> registered_events = events.get();
    StaticJsonDocument<1024> doc;
    JsonObject root = doc.to<JsonObject>();
    root["title"] = "over written title";
    JsonArray event_writer = root.createNestedArray("events");

    for_each (registered_events.begin(), registered_events.end(), [&](Event event) {
        JsonObject event_obj = event_writer.createNestedObject();
        event_obj["function_name"] = event.func_name;
        event_obj["hour"] = event.hour;
        event_obj["minute"] = event.minute;
        Serial.print(event.func_name);
        Serial.print("_");
        Serial.print(event.hour);
        Serial.print(":");
        Serial.println(event.minute);
    });

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}
