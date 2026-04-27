// api_meas.cpp
#include "api_meas.h"

static WindSensor* g_wind = nullptr;

void dht11_init(Dht11Sensor &s) {
    s.dht = new DHT(s.pin, DHT11);
    s.dht->begin();
    s.humidity = 0.0f;
    s.temperature = 0.0f;
    s.heatIndex = 0.0f;
    s.valid = false;
}

bool dht11_read(Dht11Sensor &s) {
    float h = s.dht->readHumidity();
    float t = s.dht->readTemperature();
    if (isnan(h) || isnan(t)) {
        s.valid = false;
        return false;
    }
    s.humidity = h;
    s.temperature = t;
    s.heatIndex = s.dht->computeHeatIndex(t, h, false);
    s.valid = true;
    return true;
}

void rain_init(RainSensor &r) {
    pinMode(r.pin, INPUT);
    r.raw = 0;
    r.voltage = 0.0f;
    r.rainDetected = false;
}

int rain_read_raw(RainSensor &r) {
    r.raw = analogRead(r.pin);
    return r.raw;
}

float rain_read_voltage(RainSensor &r) {
    r.raw = analogRead(r.pin);
    r.voltage = (3.3f * r.raw) / 4095.0f;
    return r.voltage;
}

bool rain_read_detected(RainSensor &r, int threshold) {
    r.raw = analogRead(r.pin);
    r.rainDetected = (r.raw >= threshold);
    return r.rainDetected;
}

void wind_init(WindSensor &w) {
    g_wind = &w;
    w.pulseCount = 0;
    pinMode(w.pin, INPUT_PULLUP);
}

void IRAM_ATTR wind_isr() {
    if (g_wind) g_wind->pulseCount++;
}

uint32_t wind_get_count() {
    if (!g_wind) return 0;
    noInterrupts();
    uint32_t c = g_wind->pulseCount;
    interrupts();
    return c;
}

void wind_reset_count() {
    if (!g_wind) return;
    noInterrupts();
    g_wind->pulseCount = 0;
    interrupts();
}