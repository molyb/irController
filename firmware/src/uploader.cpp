#include "uploader.h"


Uploader::Uploader(MonitorTemperature* monitor, WiFiClient* client) {
    monitor_ = monitor;
    client_ = client;
}

bool Uploader::enable(unsigned int channel_id, const char * write_key, uint16_t interval_sec) {
    if (!ambient_.begin(channel_id, write_key, client_)) {
        return false;
    }
    ticker_.attach_scheduled(interval_sec, std::bind(&Uploader::exec, this));
    return true;
}


void Uploader::exec(void) {
    ambient_.set(1, monitor_->getTemperature());
//    ambient_.set(1, ac.getTemperature());
    ambient_.send();
}
