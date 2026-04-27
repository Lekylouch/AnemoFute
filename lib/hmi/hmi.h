#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// =========================
// Configuration écran
// =========================
#define SCREEN_WIDTH   128
#define SCREEN_HEIGHT   64
#define SCREEN_ADDRESS 0x3C
#define OLED_RESET     -1

// =========================
// Etats système
// =========================
enum IhmAlertState {
    IHM_NORMAL = 0,
    IHM_RAIN = 1,
    IHM_WIND = 2,
    IHM_RAIN_WIND = 3,
    IHM_OVERHEAT = 4
};

enum IhmPage {
    PAGE_HOME,
    PAGE_INFO,
    PAGE_ALERT
};

// =========================
// Données IHM
// =========================
struct IhmData {
    float temperature;
    float humidity;
    int rainValue;
    uint32_t windCount;
    bool wifiConnected;
    int wifiRssi;
};

struct IhmConfig {
    uint8_t pinGreen;
    uint8_t pinOrange;
    uint8_t pinRed;
    uint8_t pinBuzzer;
    uint16_t buzzerFreq;
    uint32_t pageIntervalMs;
    uint32_t alertBeepMs;
};

// =========================
// API
// =========================
bool ihm_init(const IhmConfig &cfg);

void ihm_set_data(const IhmData &data);
void ihm_set_state(IhmAlertState state);

IhmAlertState ihm_get_state();
IhmPage ihm_get_page();

void ihm_force_page(IhmPage page);
void ihm_release_forced_page();

void ihm_update(uint32_t now);