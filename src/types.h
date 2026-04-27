#pragma once
#include <Arduino.h>

enum SystemState {
    SYS_BOOT = 0,
    SYS_NORMAL,
    SYS_RAIN,
    SYS_WIND,
    SYS_RAIN_WIND,
    SYS_OVERHEAT,
    SYS_SENSOR_ERROR
};

struct SensorData {
    float temperature;            // brute
    float temperatureFiltered;    // filtrée
    float humidity;
    float heatIndex;

    int rainRaw;
    bool rainDetected;

    uint32_t windCount;           // nb impulsions sur fenêtre
    bool windDetected;            // vent maintenu temporellement
    uint32_t lastWindDetectMs;

    bool dhtValid;
    uint32_t sampleTimeMs;
};

struct SystemContext {
    SystemState state;
    bool wifiConnected;
    bool timeValid;
    uint32_t epoch;
};

struct Thresholds {
    float tempOverheatOn;
    float tempOverheatOff;

    int rainOn;
    int rainOff;

    uint32_t windOn;
    uint32_t windOff;
};

struct ActuatorConfig {
    uint8_t voletOpenAngle;
    uint8_t voletClosedAngle;

    uint8_t fanOffLevel;
    uint8_t fanLowLevel;
    uint8_t fanHighLevel;
};