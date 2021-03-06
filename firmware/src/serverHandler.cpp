#include "serverHandler.h"
#include "MonitorTemperature.h"
// https://github.com/markszabo/IRremoteESP8266
#include <EEPROM.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <ir_Hitachi.h>
#include <ArduinoJson.h>
#include "parameters.h"
#include "saveEvent.h"
#include "eventHandler.h"
#include "RtcEvent.h"

const unsigned int ir_out_pin = IR_OUT_PIN;
extern RtcEvent rtc;

#define JSON_CAPACITY 2048

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
    File file = SPIFFS.open("/light.html", "r");
    if(!file) {
        Serial.println("Fail: load light.html");
        handleNotFound();
        return;
    }
    String html = file.readString();
    file.close();

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
    String val = server.arg("cmd");
    if (val == "on") {
        irsend.sendNEC(irPatternNecLightOn, irPatternBitsLightOn);
        state = on;
    } else if (val == "night") {
        irsend.sendNEC(irPatternNecLightNight, irPatternBitsLightNight);
        state = night;
    } else if (val == "off") {
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


// swingは実装の手間と実用面を考慮してサポートしない
// IRremoteESP8266ライブラリのIRHitachiAc424メンバ関数setSwingVToggleコメントを参照
static void controlAc(IRHitachiAc424& ac, const String& power, const String& operation_mode, const String& temp,
        const String& fan, const String& swing) {
    ac.begin();

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
}


void handleHitachiAc(void) {
    static IRHitachiAc424 ac(ir_out_pin);
    // ctrlはエアコン制御では無くチップの制御用引数
    if (server.arg("ctrl") == "get_config") {
        server.send(200, "text/plain", ac.toString());
        return ;
    }

    String power = "off";
    String operation_mode = "fan";
    String temp = "22";
    String fan = "auto";
    String swing = "off";

    power = server.arg("power");
    operation_mode = server.arg("mode");
    temp = server.arg("temp");
    fan = server.arg("fan");
    // swing = server.arg("swing");

    // この判定処理が無いと設定のためにページひらいただけでエアコンが動作してしまう
    if (power == "on" || power == "off") {
        controlAc(ac, power, operation_mode, temp, fan, swing);
    }

    File file = SPIFFS.open("/hitachiAC.html", "r");
    if(!file) {
        Serial.println("Fail: load hitachiAC.html");
        handleNotFound();
        return;
    }
    String html = file.readString();
    file.close();

    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/html", html);
}


// 以下のようなjsonファイルを生成する事を想定している
//{
//    "title": "over written title",
//    "events": [
//        {
//            "function_name": "func0",
//            "hour": 6,
//            "minute": 30,
//            "weekday": [true, false, false, false, false, false, true]
//        },
//        {
//            "function_name": "func1",
//            "hour": 8,
//            "minute": 0,
//            "weekday": [true, true, true, true, true, true, true]
//        }
//    ]
//}
static void handleConfigGet(void) {
    SaveEvent events(&EEPROM);
    std::list<Event> registered_events = events.get();
    DynamicJsonDocument doc(JSON_CAPACITY);
    JsonObject root = doc.to<JsonObject>();
    root["title"] = "IR Controller Config";
    JsonArray event_writer = root.createNestedArray("events");

    for_each (registered_events.begin(), registered_events.end(), [&](Event event) {
        JsonObject event_obj = event_writer.createNestedObject();
        if (event_obj.isNull()) {
            Serial.println("json memory is full.");
            return;
        }
        event_obj["function_name"] = event.func_name;
        event_obj["hour"] = event.hour;
        event_obj["minute"] = event.minute;
        JsonArray event_array = event_obj.createNestedArray("weekday");
        for (bool i : event.weekday) {
            event_array.add(i ? "true" : "false");
        }
    });

    String response;
    serializeJson(doc, response);
    server.send(200, "application/json", response);
}


static void handleConfigPost(void) {
    SaveEvent events(&EEPROM);
    bool update_request = false;
    // イベント追加処理 週の設定が無い場合は登録しない
    if (server.hasArg("register_event") && server.hasArg("weekday")) {
        String function_name = server.arg("register_event");
        String time = server.arg("register_time");
        int hour = time.substring(0, time.indexOf(":")).toInt();
        int minute = time.substring(time.indexOf(":") + 1).toInt();
        bool weekday[NUMBER_OF_WEEKDAY] = {false, false, false, false, false, false, false};
        // 同じname属性で複数の値を投げる実装になっているのでループを回して全部の引数について調査する
        for (int i = 0; i < server.args(); i++) {
            if (server.argName(i) == String("weekday")) {
                weekday[String(server.arg(i)).toInt()] = true;
            }
        }
        events.push(function_name, event_functions[function_name], hour, minute, weekday);
        update_request = true;
    }

    // イベント削除処理
    // 同じname属性で複数の値を投げる実装になっているのでループを回して全部の引数について調査する
    for (int i = 0; i < server.args(); i++) {
        if (server.argName(i) == String("delete_index")) {
            events.erase(server.arg(i).toInt());
            update_request = true;
        }
    }

    if (update_request) {
        rtc.clear();
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

    String file_path = "/events.html";
    if (!SPIFFS.exists(file_path)) {
        return handleConfigGet();
    }
    File file = SPIFFS.open(file_path, "r");
    server.streamFile(file, "text/html");
    file.close();
}


void handleConfig(void) {
    if (server.method() == HTTP_POST) {
        return handleConfigPost();
    } else {
        return handleConfigGet();
    }
}
