// api_meas.h
#pragma once
#include <Arduino.h>
#include <DHT.h>

struct Dht11Sensor {
    uint8_t pin;
    DHT* dht;
    float humidity;
    float temperature;
    float heatIndex;
    bool valid;
};

struct RainSensor {
    uint8_t pin;
    int raw;
    float voltage;
    bool rainDetected;
};

struct WindSensor {
    uint8_t pin;
    volatile uint32_t pulseCount;
};

void dht11_init(Dht11Sensor &s);
bool dht11_read(Dht11Sensor &s);

void rain_init(RainSensor &r);
int rain_read_raw(RainSensor &r);
float rain_read_voltage(RainSensor &r);
bool rain_read_detected(RainSensor &r, int threshold);

void wind_init(WindSensor &w);
void IRAM_ATTR wind_isr();
uint32_t wind_get_count();
void wind_reset_count();