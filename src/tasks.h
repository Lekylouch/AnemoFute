#pragma once

#include <Arduino.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>

#include "types.h"
#include "config.h"
#include "api_meas.h"
#include "api_wifi.h"
#include "hmi.h"
#include "actuators.h"
#include "fsm.h"

extern SensorData g_sensorData;
extern SystemContext g_systemCtx;

extern SemaphoreHandle_t g_sensorMutex;
extern SemaphoreHandle_t g_ctxMutex;

extern ServoMoteur g_volet;
extern FanCtrl g_fan;

void create_app_tasks();