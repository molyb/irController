#ifndef FIRMWARE_SAVENETWORKCONFIG_H
#define FIRMWARE_SAVENETWORKCONFIG_H



#include <Arduino.h>
#include <EEPROM.h>

#define SAVE_NETWORK_CONFIG_SSID_ADDRESS 0x00  // 32 byte
#define SAVE_NETWORK_CONFIG_SSID_LENGTH (32 + 1)  // tail char is null
#define SAVE_NETWORK_CONFIG_PASSWORD_ADDRESS (SAVE_NETWORK_CONFIG_SSID_ADDRESS + SAVE_NETWORK_CONFIG_SSID_LENGTH)  // 63 byte
#define SAVE_NETWORK_CONFIG_PASSWORD_LENGTH (63 + 1)  // tail char is null

class SaveNetworkConfig {
public:
    SaveNetworkConfig(EEPROMClass *eeprom);
    bool setSsid(String ssid);
    bool setPassword(String password);
    String ssid(void);
    String password(void);

private:
    EEPROMClass *eeprom_;
};


#endif //FIRMWARE_SAVENETWORKCONFIG_H
