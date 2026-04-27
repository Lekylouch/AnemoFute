#include "tasks.h"

SensorData g_sensorData = {};
SystemContext g_systemCtx = { SYS_BOOT, false, false, 0 };

SemaphoreHandle_t g_sensorMutex = nullptr;
SemaphoreHandle_t g_ctxMutex = nullptr;

ServoMoteur g_volet = {
    .pin = PIN_SERVO,
    .min_pulse = 500,
    .max_pulse = 2400,
    .freq = 50,
    .currentAngle = 0,
    .attached = false
};

FanCtrl g_fan = {
    .pin = PIN_FAN,
    .pwm = false,
    .level = 0,
    .channel = 0
};

static void SensorsTask(void *pv);
static void ControlTask(void *pv);
static void WifiTask(void *pv);

void create_app_tasks() {
    g_sensorMutex = xSemaphoreCreateMutex();
    g_ctxMutex = xSemaphoreCreateMutex();

    xTaskCreate(SensorsTask, "Sensors", 4096, nullptr, 3, nullptr);
    xTaskCreate(ControlTask, "Control", 4096, nullptr, 2, nullptr);
    xTaskCreate(WifiTask, "Wifi", 6144, nullptr, 1, nullptr);
}

void SensorsTask(void *pv) {
    (void)pv;

    Dht11Sensor dht = { PIN_DHT, nullptr, 0, 0, 0, false };
    RainSensor rain = { PIN_RAIN, 0, 0.0f, false };
    WindSensor wind = { PIN_WIND, 0 };

    dht11_init(dht);
    rain_init(rain);
    wind_init(wind);

    attachInterrupt(digitalPinToInterrupt(wind.pin), wind_isr, FALLING);

    SensorData local = {};
    local.temperature = 0.0f;
    local.temperatureFiltered = 0.0f;
    local.humidity = 0.0f;
    local.heatIndex = 0.0f;
    local.rainRaw = 0;
    local.rainDetected = false;
    local.windCount = 0;
    local.windDetected = false;
    local.lastWindDetectMs = 0;
    local.dhtValid = false;
    local.sampleTimeMs = millis();

    uint32_t lastDhtReadMs = 0;
    uint32_t lastRainReadMs = 0;
    uint32_t lastWindWindowMs = 0;

    const uint32_t dhtPeriodMs = 2000;
    const uint32_t rainPeriodMs = 500;
    const uint32_t windWindowMs = 2000;
    const uint32_t windHoldMs = 3000;

    for (;;) {
        uint32_t now = millis();

        // ---- DHT ----
        if (now - lastDhtReadMs >= dhtPeriodMs) {
            lastDhtReadMs = now;

            bool ok = dht11_read(dht);
            local.dhtValid = ok && dht.valid;

            if (local.dhtValid) {
                local.temperature = dht.temperature;
                local.humidity = dht.humidity;
                local.heatIndex = dht.heatIndex;

                if (local.temperatureFiltered == 0.0f) {
                    local.temperatureFiltered = local.temperature;
                } else {
                    local.temperatureFiltered =
                        0.8f * local.temperatureFiltered +
                        0.2f * local.temperature;
                }
            }
        }

        // ---- Rain ----
        if (now - lastRainReadMs >= rainPeriodMs) {
            lastRainReadMs = now;

            rain_read_raw(rain);
            local.rainRaw = rain.raw;

            if (local.rainRaw >= g_thresholds.rainOn) {
                local.rainDetected = true;
            } else if (local.rainRaw <= g_thresholds.rainOff) {
                local.rainDetected = false;
            }
        }

        // ---- Wind ----
        if (now - lastWindWindowMs >= windWindowMs) {
            lastWindWindowMs = now;

            uint32_t count = wind_get_count();
            wind_reset_count();

            local.windCount = count;

            if (count >= g_thresholds.windOn) {
                local.windDetected = true;
                local.lastWindDetectMs = now;
            } else if ((now - local.lastWindDetectMs) > windHoldMs) {
                local.windDetected = false;
            }
        }

        local.sampleTimeMs = now;

        xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
        g_sensorData = local;
        xSemaphoreGive(g_sensorMutex);

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void ControlTask(void *pv) {
    (void)pv;

    IhmConfig cfg = {
        .pinGreen = PIN_LED_GREEN,
        .pinOrange = PIN_LED_ORANGE,
        .pinRed = PIN_LED_RED,
        .pinBuzzer = PIN_BUZZER,
        .buzzerFreq = 2000,
        .pageIntervalMs = 3000,
        .alertBeepMs = 300
    };

    bool ihmOk = ihm_init(cfg);
    bool servoOk = servo_init(g_volet);
    bool fanOk = fan_init(g_fan);

    Serial.print("[INIT] IHM=");
    Serial.print(ihmOk);
    Serial.print(" SERVO=");
    Serial.print(servoOk);
    Serial.print(" FAN=");
    Serial.println(fanOk);

    SystemState lastState = SYS_BOOT;

    for (;;) {
        uint32_t now = millis();

        SensorData local;
        xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
        local = g_sensorData;
        xSemaphoreGive(g_sensorMutex);

        SystemContext ctxLocal;
        xSemaphoreTake(g_ctxMutex, portMAX_DELAY);
        ctxLocal = g_systemCtx;
        xSemaphoreGive(g_ctxMutex);

        SystemState st = compute_system_state(local, g_thresholds);

        xSemaphoreTake(g_ctxMutex, portMAX_DELAY);
        g_systemCtx.state = st;
        ctxLocal = g_systemCtx;
        xSemaphoreGive(g_ctxMutex);

        apply_actuators(st, local, g_volet, g_fan, g_actuatorCfg);
        servo_update(g_volet, now);

        update_ihm_from_state(st, local, ctxLocal);
        ihm_update(now);

        if (st != lastState) {
            lastState = st;
            Serial.print("[FSM] state=");
            Serial.print((int)st);
            Serial.print(" T=");
            Serial.print(local.temperature, 1);
            Serial.print(" Tf=");
            Serial.print(local.temperatureFiltered, 1);
            Serial.print(" H=");
            Serial.print(local.humidity, 1);
            Serial.print(" Rain=");
            Serial.print(local.rainRaw);
            Serial.print(" WindCnt=");
            Serial.print(local.windCount);
            Serial.print(" WindDet=");
            Serial.println(local.windDetected ? 1 : 0);
        }

        vTaskDelay(pdMS_TO_TICKS(100));
    }
}


void WifiTask(void *pv) {
    (void)pv;

    WifiServiceConfig wifiCfg = {
        .ssid = "AnemoFute@1234",
        .password = "123456789",
        .ubidotsToken = "BBUS-NQS9PBe0RHKGJ5lPK9ghLbAJQ19NbM",
        .deviceLabel = "AnemoFute",
        .reconnectIntervalMs = 10000,
        .publishIntervalMs = 30000,
        .gmtOffsetSec = 3600,
        .daylightOffsetSec = 3600
    };

    wifi_service_init(wifiCfg);

    for (;;) {
        SensorData local;
        xSemaphoreTake(g_sensorMutex, portMAX_DELAY);
        local = g_sensorData;
        xSemaphoreGive(g_sensorMutex);

        NetData net;
        net.temperature = (local.temperatureFiltered > 0.0f)
                        ? local.temperatureFiltered
                        : local.temperature;
        net.humidity = local.humidity;
        net.rainValue = local.rainRaw;
        net.windCount = local.windCount;

        wifi_service_set_data(net);
        wifi_service_update(millis());

        xSemaphoreTake(g_ctxMutex, portMAX_DELAY);
        g_systemCtx.wifiConnected = wifi_service_is_connected();
        g_systemCtx.timeValid = wifi_service_time_valid();
        g_systemCtx.epoch = wifi_service_get_epoch();
        xSemaphoreGive(g_ctxMutex);

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}