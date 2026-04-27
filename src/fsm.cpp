#include "fsm.h"

// --------------------------------------------------
// Calcul de l'état système
// --------------------------------------------------
SystemState compute_system_state(const SensorData& s, const Thresholds& th) {
    // Si le capteur DHT n'est pas valide, on considère une erreur capteur
    if (!s.dhtValid) {
        return SYS_SENSOR_ERROR;
    }

    // Latches internes pour hystérésis
    static bool rainLatch = false;
    static bool hotLatch = false;

    // Température utilisée par la FSM
    float temp = (s.temperatureFiltered > 0.0f)
               ? s.temperatureFiltered
               : s.temperature;

    // ---------- Pluie avec hystérésis ----------
    if (s.rainRaw >= th.rainOn) {
        rainLatch = true;
    } else if (s.rainRaw <= th.rainOff) {
        rainLatch = false;
    }

    // ---------- Surchauffe avec hystérésis ----------
    if (temp >= th.tempOverheatOn) {
        hotLatch = true;
    } else if (temp <= th.tempOverheatOff) {
        hotLatch = false;
    }

    // ---------- Vent ----------
    bool windActive = s.windDetected;

    // ---------- Priorités ----------
    if (hotLatch) {
        return SYS_OVERHEAT;
    }

    if (rainLatch && windActive) {
        return SYS_RAIN_WIND;
    }

    if (rainLatch) {
        return SYS_RAIN;
    }

    if (windActive) {
        return SYS_WIND;
    }

    return SYS_NORMAL;
}

// --------------------------------------------------
// Pilotage des actionneurs
// --------------------------------------------------
void apply_actuators(SystemState st,
                     const SensorData& s,
                     ServoMoteur& volet,
                     FanCtrl& fan,
                     const ActuatorConfig& cfg) {
    // -------- Servo / volet --------
    switch (st) {
        case SYS_RAIN:
        case SYS_WIND:
        case SYS_RAIN_WIND:
        case SYS_SENSOR_ERROR:
            // météo défavorable ou erreur -> fermeture
            servo_set_target_angle(volet, cfg.voletClosedAngle);
            break;

        case SYS_NORMAL:
        case SYS_OVERHEAT:
        case SYS_BOOT:
        default:
            // normal ou surchauffe -> ouvert
            // (la ventilation dépend du ventilo, pas forcément du volet)
            servo_set_target_angle(volet, cfg.voletOpenAngle);
            break;
    }

    // -------- Ventilateur --------
    switch (st) {
        case SYS_OVERHEAT:
            // en surchauffe -> vitesse max
            fan_set_level(fan, cfg.fanHighLevel);
            break;

        case SYS_SENSOR_ERROR:
            // en cas d'erreur capteur -> OFF par prudence
            fan_set_level(fan, cfg.fanOffLevel);
            break;

        case SYS_NORMAL:
        case SYS_RAIN:
        case SYS_WIND:
        case SYS_RAIN_WIND:
        case SYS_BOOT:
        default: {
            // Mode intermédiaire optionnel
            float temp = (s.temperatureFiltered > 0.0f)
                       ? s.temperatureFiltered
                       : s.temperature;

            if (temp >= 32.0f) {
                fan_set_level(fan, cfg.fanLowLevel);
            } else {
                fan_set_level(fan, cfg.fanOffLevel);
            }
            break;
        }
    }
}

// --------------------------------------------------
// Conversion état système -> IHM
// --------------------------------------------------
void update_ihm_from_state(SystemState st,
                           const SensorData& s,
                           const SystemContext& ctx) {
    IhmData data;
    data.temperature = (s.temperatureFiltered > 0.0f)
                     ? s.temperatureFiltered
                     : s.temperature;
    data.humidity = s.humidity;
    data.rainValue = s.rainRaw;
    data.windCount = s.windCount;
    data.wifiConnected = ctx.wifiConnected;
    data.wifiRssi = ctx.wifiConnected ? -60 : -90;

    ihm_set_data(data);

    IhmAlertState alert = IHM_NORMAL;

    switch (st) {
        case SYS_NORMAL:
            alert = IHM_NORMAL;
            break;

        case SYS_RAIN:
            alert = IHM_RAIN;
            break;

        case SYS_WIND:
            alert = IHM_WIND;
            break;

        case SYS_RAIN_WIND:
            alert = IHM_RAIN_WIND;
            break;

        case SYS_OVERHEAT:
            alert = IHM_OVERHEAT;
            break;

        case SYS_SENSOR_ERROR:
            // Tu peux créer un état IHM spécifique plus tard si tu veux
            alert = IHM_OVERHEAT;
            break;

        case SYS_BOOT:
        default:
            alert = IHM_NORMAL;
            break;
    }

    ihm_set_state(alert);
}