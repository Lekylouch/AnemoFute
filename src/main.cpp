#include <Arduino.h>
#include "tasks.h"

void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("AnemoFute boot...");

    create_app_tasks();
}

void loop() {
    vTaskDelay(pdMS_TO_TICKS(1000));
} 

/*
#include <ESP32Servo.h>

#define PIN_SG90 2

typedef struct {
  Servo servo;
  uint8_t pin;
  uint16_t freq;
  uint16_t min_pulse;
  uint16_t max_pulse;
} servomoteur;

servomoteur s = {
  .pin = PIN_SG90,
  .freq = 50,
  .min_pulse = 500,
  .max_pulse = 2400
};

void config_servo(servomoteur &m) {
  m.servo.setPeriodHertz(m.freq);
  m.servo.attach(m.pin, m.min_pulse, m.max_pulse);
}

void set_position_ang(servomoteur &m, uint8_t angle) {
  m.servo.write(angle);
}

void setup() {
  Serial.begin(115200);

  config_servo(s);
  delay(200);

  set_position_ang(s, 0);
  Serial.println("Servo initialise");
  delay(1000);
}

void loop() {
  for (int pos = 0; pos <= 180; pos++) {
    set_position_ang(s, pos);
    delay(10);
  }

  for (int pos = 180; pos >= 0; pos--) {
    set_position_ang(s, pos);
    delay(10);
  }

  Serial.println("jdjdjd");
}
*/
