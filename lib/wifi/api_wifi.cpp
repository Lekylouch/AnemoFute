#include "api_wifi.h"

#include <WiFi.h>
#include <HTTPClient.h>
#include <time.h>

static WifiServiceConfig g_cfg;
static NetData g_data = {0.0f, 0.0f, 0, 0};

static bool g_initialized = false;
static bool g_timeValid = false;

static uint32_t g_lastReconnectAttempt = 0;
static uint32_t g_lastPublishAttempt = 0;

static int g_lastHttpCode = 0;

// --------------------------------------------------
// Outils internes
// --------------------------------------------------
static bool time_is_valid() {
    time_t now = time(nullptr);
    return (now > 1700000000); // seuil simple : date réaliste après ~2023
}

static void start_wifi() {
    WiFi.mode(WIFI_STA);
    WiFi.begin(g_cfg.ssid, g_cfg.password);
}

static void update_time_status() {
    g_timeValid = time_is_valid();
}

static void ensure_time_configured() {
    static bool configured = false;
    if (!configured) {
        configTime(g_cfg.gmtOffsetSec, g_cfg.daylightOffsetSec, "pool.ntp.org", "time.nist.gov");
        configured = true;
    }
}

static String build_ubidots_payload(uint32_t ts) {
    String payload = "{";

    payload += "\"temperature\":{\"value\":";
    payload += String(g_data.temperature, 2);
    payload += ",\"timestamp\":";
    payload += String(ts);
    payload += "},";

    payload += "\"humidity\":{\"value\":";
    payload += String(g_data.humidity, 2);
    payload += ",\"timestamp\":";
    payload += String(ts);
    payload += "},";

    payload += "\"rain\":{\"value\":";
    payload += String(g_data.rainValue);
    payload += ",\"timestamp\":";
    payload += String(ts);
    payload += "},";

    payload += "\"wind\":{\"value\":";
    payload += String(g_data.windCount);
    payload += ",\"timestamp\":";
    payload += String(ts);
    payload += "}";

    payload += "}";

    return payload;
}

static bool publish_to_ubidots(uint32_t ts) {
    if (WiFi.status() != WL_CONNECTED) {
        g_lastHttpCode = -100;
        return false;
    }

    HTTPClient http;

    String url = "http://industrial.api.ubidots.com/api/v1.6/devices/";
    url += g_cfg.deviceLabel;

    http.begin(url);
    http.addHeader("Content-Type", "application/json");
    http.addHeader("X-Auth-Token", g_cfg.ubidotsToken);

    String payload = build_ubidots_payload(ts);

    int httpCode = http.POST(payload);
    g_lastHttpCode = httpCode;

    if (httpCode > 0) {
        String response = http.getString();
        (void)response;
    }

    http.end();
    return (httpCode >= 200 && httpCode < 300);
}

// --------------------------------------------------
// API publique
// --------------------------------------------------
bool wifi_service_init(const WifiServiceConfig& cfg) {
    g_cfg = cfg;
    g_initialized = true;
    g_timeValid = false;
    g_lastReconnectAttempt = 0;
    g_lastPublishAttempt = 0;
    g_lastHttpCode = 0;

    start_wifi();
    ensure_time_configured();

    return true;
}

void wifi_service_set_data(const NetData& data) {
    g_data = data;
}

void wifi_service_update(uint32_t now) {
    if (!g_initialized) return;

    if (WiFi.status() != WL_CONNECTED) {
        g_timeValid = false;

        if (now - g_lastReconnectAttempt >= g_cfg.reconnectIntervalMs) {
            g_lastReconnectAttempt = now;

            WiFi.disconnect();
            start_wifi();
        }
        return;
    }

    ensure_time_configured();
    update_time_status();

    if (now - g_lastPublishAttempt >= g_cfg.publishIntervalMs) {
        g_lastPublishAttempt = now;

        uint32_t ts = 0;
        if (g_timeValid) {
            ts = (uint32_t)time(nullptr);
        }

        publish_to_ubidots(ts);
    }
}

bool wifi_service_is_connected() {
    return (WiFi.status() == WL_CONNECTED);
}

bool wifi_service_time_valid() {
    return g_timeValid;
}

uint32_t wifi_service_get_epoch() {
    if (!g_timeValid) return 0;
    return (uint32_t)time(nullptr);
}

String wifi_service_get_time_string() {
    if (!g_timeValid) {
        return "TIME_INVALID";
    }

    time_t now = time(nullptr);
    struct tm timeinfo;
    localtime_r(&now, &timeinfo);

    char buffer[32];
    strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
    return String(buffer);
}

bool wifi_service_publish_now() {
    if (!g_initialized) return false;
    if (WiFi.status() != WL_CONNECTED) return false;

    update_time_status();

    uint32_t ts = 0;
    if (g_timeValid) {
        ts = (uint32_t)time(nullptr);
    }

    bool ok = publish_to_ubidots(ts);
    g_lastPublishAttempt = millis();
    return ok;
}

int wifi_service_get_last_http_code() {
    return g_lastHttpCode;
}