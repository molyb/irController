#include "serverHandler.h"
#include "MonitorTemperature.h"
// https://github.com/markszabo/IRremoteESP8266
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <IRrecv.h>
#include <ir_hitachi.h>
#include "parameters.h"


const unsigned int ir_out_pin = IR_OUT_PIN;

void handleRoot(void) {
    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/plain", "IR Controller");
}

void handleTemperature(void) {
    monitor.update();
    float temperature = monitor.temperature();

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

void handleLight(void) {
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
    enum State state = unknown;
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

    auto state2str = [](enum State state) {
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

    String header = "\
<html lang=\"ja\">\n\
    <meta charset=\"utf-8\">\n\
    <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n\
    <head>\n\
    <title>IrController</title>\n\
</head>\n\
  <title>IrController</title>\n\
</head>\n";

    String body = "\
<body style=\"font-family: sans-serif; background-color: #ffffff;\" >\n\
  <h1>Light Controller</h1>\n\
  <p>";
    body += "\
    current state: " + state2str(state) + "\n";
    body += "\
    <form action='' method='post'>\n\
      <button name='light' value='on'>On</button>\n\
    </form>\n\
    <form action='' method='post'>\n\
      <button name='light' value='night'>Night</button>\n\
    </form>\n\
    <form action='' method='post'>\n\
      <button name='light' value='off'>Off</button>\n\
    </form>\n\
  </p>\n\
</body>\n";

    String footer = "</html>\n";
    String message = header + body + footer;

    server.send(200, "text/html", message);
}

void handleHitachiAc(void) {
    IRHitachiAc424 ac(ir_out_pin);
    ac.begin();

    String message = "AC Controller\n";
    String power = "off";
    String operation_mode = "fan";
    String temp = "22";
    String fan = "auto";
    String swing = "off";

    // swingは実装の手間と実用面を考慮してサポートしない
    // IRremoteESP8266ライブラリのIRHitachiAc424メンバ関数setSwingVToggleコメントを参照
    for (uint8_t i = 0; i < server.args(); i++) {
        if (server.argName(i) == "power") {
            power = server.arg(i);
        } else if (server.argName(i) == "mode") {
            operation_mode = server.arg(i);
        } else  if (server.argName(i) == "temp") {
            temp = server.arg(i);
        } else if (server.argName(i) == "fan") {
            fan = server.arg(i);
        } else {
            // do nothing
        }
    }

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
        // IRHitachiAc424にはオートが無い
        message += ("Selected invalid mode:" + operation_mode + "\n");
        message += ("Please select the righ mode. [fan, cool, heat, dry]\n");
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
        ac.setFan(kHitachiAc424FanAuto);
    }
    ac.setButton(kHitachiAc424ButtonPowerMode);
    ac.send();
    message += "\n";
    message += ac.toString();
    // HTTPステータスコード(200) リクエストの成功
    server.send(200, "text/plain", message);
}
