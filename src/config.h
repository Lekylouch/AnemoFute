#pragma once
#include <Arduino.h>
#include "types.h"

// Pins capteurs
#define PIN_DHT      D7
#define PIN_RAIN     D6
#define PIN_WIND     A2

// Pins actionneurs / IHM
#define PIN_SERVO    A1
#define PIN_FAN      A3
#define PIN_LED_GREEN  D2
#define PIN_LED_ORANGE D3
#define PIN_LED_RED    D4
#define PIN_BUZZER     D5

// Seuils
static const Thresholds g_thresholds = {
    .tempOverheatOn  = 28.0f,
    .tempOverheatOff = 28.0f,
    .rainOn          = 1000,
    .rainOff         = 800,
    .windOn          = 10,
    .windOff         = 5
};



// =========================
// Configuration actionneurs
// =========================
static const ActuatorConfig g_actuatorCfg = {
    .voletOpenAngle   = 180,
    .voletClosedAngle = 0,

    .fanOffLevel      = 0,
    .fanLowLevel      = 120,
    .fanHighLevel     = 255
};