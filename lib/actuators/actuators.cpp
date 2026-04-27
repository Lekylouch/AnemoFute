#include "actuators.h"

// =========================
// Servo
// =========================
static uint8_t clamp_angle(uint8_t angle) {
    if (angle > 180) return 180;
    return angle;
}

bool servo_init(ServoMoteur &m) {
    m.currentAngle = 0;
    m.attached = false;

    m.servo.setPeriodHertz(m.freq);
    int ok = m.servo.attach(m.pin, m.min_pulse, m.max_pulse);
    if (ok == 0) {
        return false;
    }

    m.servo.write(0);
    m.currentAngle = 0;
    m.attached = true;
    return true;
}

void servo_detach(ServoMoteur &m) {
    if (m.attached) {
        m.servo.detach();
        m.attached = false;
    }
}

void servo_set_angle(ServoMoteur &m, uint8_t angle) {
    if (!m.attached) return;
    angle = clamp_angle(angle);
    m.servo.write(angle);
    m.currentAngle = angle;
}

uint8_t servo_get_angle(const ServoMoteur &m) {
    return m.currentAngle;
}

void servo_set_target_angle(ServoMoteur &m, uint8_t targetAngle) {
    servo_set_angle(m, targetAngle);
}

void servo_update(ServoMoteur &m, uint32_t now) {
    (void)m;
    (void)now;
}

// =========================
// Fan
// =========================
bool fan_init(FanCtrl &f) {
    f.level = 0;

    if (f.pwm) {
        const int freq = 25000;
        const int resolution = 8;
        ledcSetup(f.channel, freq, resolution);
        ledcAttachPin(f.pin, f.channel);
        ledcWrite(f.channel, 0);
    } else {
        pinMode(f.pin, OUTPUT);
        digitalWrite(f.pin, LOW);
    }

    return true;
}

void fan_set_level(FanCtrl &f, uint8_t level) {
    f.level = level;

    if (f.pwm) {
        ledcWrite(f.channel, level);
    } else {
        digitalWrite(f.pin, level ? HIGH : LOW);
    }
}

uint8_t fan_get_level(const FanCtrl &f) {
    return f.level;
}