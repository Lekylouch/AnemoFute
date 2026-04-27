#pragma once
#include "types.h"
#include "actuators.h"
#include "hmi.h"


bool fan_init(FanCtrl &f);
void fan_set_level(FanCtrl &f, uint8_t level);
uint8_t fan_get_level(const FanCtrl &f);

SystemState compute_system_state(const SensorData& s, const Thresholds& th);

void apply_actuators(SystemState st,
                     const SensorData& s,
                     ServoMoteur& volet,
                     FanCtrl& fan,
                     const ActuatorConfig& cfg);

void update_ihm_from_state(SystemState st,
                           const SensorData& s,
                           const SystemContext& ctx);