#pragma once

#include <Arduino.h>

struct NetData {
    float temperature;
    float humidity;
    int rainValue;
    uint32_t windCount;
};

struct WifiServiceConfig {
    const char* ssid;
    const char* password;
    const char* ubidotsToken;
    const char* deviceLabel;

    uint32_t reconnectIntervalMs;
    uint32_t publishIntervalMs;

    long gmtOffsetSec;
    int daylightOffsetSec;
};

bool wifi_service_init(const WifiServiceConfig& cfg);

void wifi_service_set_data(const NetData& data);
void wifi_service_update(uint32_t now);

bool wifi_service_is_connected();
bool wifi_service_time_valid();

uint32_t wifi_service_get_epoch();
String wifi_service_get_time_string();

bool wifi_service_publish_now();
int wifi_service_get_last_http_code();