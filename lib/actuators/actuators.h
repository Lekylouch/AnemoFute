#pragma once
#include <Arduino.h>
#include <ESP32Servo.h>

// =========================
// Servo
// =========================
struct ServoMoteur {
    uint8_t pin;
    uint16_t min_pulse;
    uint16_t max_pulse;
    uint16_t freq;
    uint8_t currentAngle;
    bool attached;
    Servo servo;
};

bool servo_init(ServoMoteur &m);
void servo_detach(ServoMoteur &m);
void servo_set_angle(ServoMoteur &m, uint8_t angle);
uint8_t servo_get_angle(const ServoMoteur &m);
void servo_set_target_angle(ServoMoteur &m, uint8_t targetAngle);
void servo_update(ServoMoteur &m, uint32_t now);

// =========================
// Fan
// =========================
struct FanCtrl {
    uint8_t pin;
    bool pwm;
    uint8_t level;
    uint8_t channel;
};

bool fan_init(FanCtrl &f);
void fan_set_level(FanCtrl &f, uint8_t level);
uint8_t fan_get_level(const FanCtrl &f);