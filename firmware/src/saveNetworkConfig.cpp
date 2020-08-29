#include "saveNetworkConfig.h"


SaveNetworkConfig::SaveNetworkConfig(EEPROMClass *eeprom) {
    eeprom_ = eeprom;
}

bool SaveNetworkConfig::setSsid(String ssid) {
    ssid.trim();  // 自身をtrimする（returnがvoid）
    ssid.concat('\0');  // add null
    char ssid_c[SAVE_NETWORK_CONFIG_SSID_LENGTH] = {0};
    // 32文字以上だった場合のトリミングも兼ねる（そもそも32文字以上は異常系だが）
    ssid.toCharArray(ssid_c, SAVE_NETWORK_CONFIG_SSID_LENGTH - 1);
    eeprom_->put<char[SAVE_NETWORK_CONFIG_SSID_LENGTH]>(SAVE_NETWORK_CONFIG_SSID_ADDRESS, ssid_c);
    return eeprom_->commit();
}

bool SaveNetworkConfig::SaveNetworkConfig::setPassword(String password) {
    password.trim();  // 自身をtrimする（returnがvoid）
    password.concat('\0');  // add null
    char password_c[SAVE_NETWORK_CONFIG_PASSWORD_LENGTH] = {0};
    // 63文字以上だった場合のトリミングも兼ねる（そもそも63文字以上は異常系だが）
    password.toCharArray(password_c, SAVE_NETWORK_CONFIG_PASSWORD_LENGTH - 1);
    eeprom_->put<char[SAVE_NETWORK_CONFIG_PASSWORD_LENGTH]>(SAVE_NETWORK_CONFIG_PASSWORD_ADDRESS, password_c);
    return eeprom_->commit();
}

String SaveNetworkConfig::ssid(void) {
    char saved_ssid[SAVE_NETWORK_CONFIG_SSID_LENGTH];
    eeprom_->get<char[SAVE_NETWORK_CONFIG_SSID_LENGTH]>(SAVE_NETWORK_CONFIG_SSID_ADDRESS, saved_ssid);
    // 末尾に終端文字を追加
    char ssid[SAVE_NETWORK_CONFIG_SSID_LENGTH] = {'\0'};
    // EEPROM書き込み時も保護を入れているが万が一の破壊に備えて
    for (int i = 0; i < SAVE_NETWORK_CONFIG_SSID_LENGTH - 1; i++) {
        ssid[i] = saved_ssid[i];
    }
    return String(ssid);
}

String SaveNetworkConfig::password(void) {
    char saved_password[SAVE_NETWORK_CONFIG_PASSWORD_LENGTH];
    eeprom_->get<char[SAVE_NETWORK_CONFIG_PASSWORD_LENGTH]>(SAVE_NETWORK_CONFIG_PASSWORD_ADDRESS, saved_password);
    // 末尾に終端文字を追加
    char password[SAVE_NETWORK_CONFIG_PASSWORD_LENGTH] = {'\0'};
    // EEPROM書き込み時も保護を入れているが万が一の破壊に備えて
    for (int i = 0; i < SAVE_NETWORK_CONFIG_PASSWORD_LENGTH - 1; i++) {
        password[i] = saved_password[i];
    }
    return String(password);
}
