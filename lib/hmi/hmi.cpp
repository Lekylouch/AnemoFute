#include "hmi.h"

static Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

static IhmConfig g_cfg;
static IhmData g_data = {0.0f, 0.0f, 0, 0, false, 0};
static IhmAlertState g_state = IHM_NORMAL;
static IhmPage g_page = PAGE_HOME;

static bool g_forcePage = false;
static IhmPage g_forcedPage = PAGE_HOME;

static uint32_t g_lastPageSwitch = 0;
static uint32_t g_lastBuzzToggle = 0;
static bool g_buzzerOn = false;
static bool g_initialized = false;

// =========================
// Outils internes
// =========================
static void led_write(bool g, bool o, bool r) {
    digitalWrite(g_cfg.pinGreen,  g ? HIGH : LOW);
    digitalWrite(g_cfg.pinOrange, o ? HIGH : LOW);
    digitalWrite(g_cfg.pinRed,    r ? HIGH : LOW);
}

static void buzzer_on() {
#if defined(ESP32)
    tone(g_cfg.pinBuzzer, g_cfg.buzzerFreq);
#else
    tone(g_cfg.pinBuzzer, g_cfg.buzzerFreq);
#endif
    g_buzzerOn = true;
}

static void buzzer_off() {
    noTone(g_cfg.pinBuzzer);
    g_buzzerOn = false;
}

static void update_leds() {
    switch (g_state) {
        case IHM_NORMAL:     led_write(false,  true, true); break;
        case IHM_RAIN:       led_write(true, false,  true); break;
        case IHM_WIND:       led_write(true, true, false ); break;
        case IHM_RAIN_WIND:  led_write(true, false,  false ); break;
        case IHM_OVERHEAT:   led_write(true, true, false ); break;
        default:             led_write(false, false, false); break;
    }
}

static void update_buzzer(uint32_t now) {
    if (g_state == IHM_NORMAL) {
        buzzer_off();
        return;
    }

    uint32_t interval = g_cfg.alertBeepMs;
    if (g_state == IHM_OVERHEAT) {
        interval = g_cfg.alertBeepMs / 2;
        if (interval < 80) interval = 80;
    }

    if (now - g_lastBuzzToggle >= interval) {
        g_lastBuzzToggle = now;
        if (g_buzzerOn) buzzer_off();
        else buzzer_on();
    }
}

static void update_page(uint32_t now) {
    if (g_forcePage) {
        g_page = g_forcedPage;
        return;
    }

    if (g_state != IHM_NORMAL) {
        g_page = PAGE_ALERT;
        return;
    }

    if (now - g_lastPageSwitch >= g_cfg.pageIntervalMs) {
        g_lastPageSwitch = now;
        g_page = (g_page == PAGE_HOME) ? PAGE_INFO : PAGE_HOME;
    }
}

static const char* state_to_text(IhmAlertState s) {
    switch (s) {
        case IHM_NORMAL:    return "NORMAL";
        case IHM_RAIN:      return "PLUIE";
        case IHM_WIND:      return "VENT";
        case IHM_RAIN_WIND: return "VENT+PLUIE";
        case IHM_OVERHEAT:  return "SURCHAUFFE";
        default:            return "INCONNU";
    }
}

static void draw_wifi_line() {
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);
    display.setCursor(0, 0);

    if (g_data.wifiConnected) {
        display.print("WiFi: OK ");
        display.print(g_data.wifiRssi);
        display.print(" dBm");
    } else {
        display.print("WiFi: OFF");
    }
}

static void draw_home() {
    display.clearDisplay();

    draw_wifi_line();

    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print("AnemoFute");

    display.setTextSize(1);
    display.setCursor(0, 44);
    display.print("Etat: ");
    display.print(state_to_text(g_state));

    display.display();
}

static void draw_info() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print("Temp: ");
    display.print(g_data.temperature, 1);
    display.print(" C");

    display.setCursor(0, 12);
    display.print("Hum : ");
    display.print(g_data.humidity, 1);
    display.print(" %");

    display.setCursor(0, 24);
    display.print("Pluie: ");
    display.print(g_data.rainValue);

    display.setCursor(0, 36);
    display.print("Vent : ");
    display.print(g_data.windCount);

    display.setCursor(0, 52);
    display.print("Etat : ");
    display.print(state_to_text(g_state));

    display.display();
}

static void draw_alert() {
    display.clearDisplay();

    display.setTextSize(1);
    display.setCursor(0, 0);
    display.print("!!! ALERTE !!!");

    display.setTextSize(2);
    display.setCursor(0, 18);
    display.print(state_to_text(g_state));

    display.setTextSize(1);
    display.setCursor(0, 46);

    switch (g_state) {
        case IHM_RAIN:
            display.print("Pluie detectee");
            break;
        case IHM_WIND:
            display.print("Vent fort detecte");
            break;
        case IHM_RAIN_WIND:
            display.print("Pluie + vent fort");
            break;
        case IHM_OVERHEAT:
            display.print("Temperature critique");
            break;
        default:
            display.print("Verification requise");
            break;
    }

    display.display();
}

static void render_page() {
    switch (g_page) {
        case PAGE_HOME:  draw_home();  break;
        case PAGE_INFO:  draw_info();  break;
        case PAGE_ALERT: draw_alert(); break;
        default:         draw_home();  break;
    }
}

// =========================
// API publique
// =========================
bool ihm_init(const IhmConfig &cfg) {
    g_cfg = cfg;

    pinMode(g_cfg.pinGreen, OUTPUT);
    pinMode(g_cfg.pinOrange, OUTPUT);
    pinMode(g_cfg.pinRed, OUTPUT);
    pinMode(g_cfg.pinBuzzer, OUTPUT);

    led_write(false, false, false);
    buzzer_off();

    if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
        return false;
    }

    display.clearDisplay();
    display.setTextColor(SSD1306_WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("IHM init...");
    display.display();

    g_lastPageSwitch = millis();
    g_lastBuzzToggle = millis();
    g_initialized = true;
    return true;
}

void ihm_set_data(const IhmData &data) {
    g_data = data;
}

void ihm_set_state(IhmAlertState state) {
    if (g_state != state) {
        g_state = state;
        g_lastBuzzToggle = millis();
        if (g_state == IHM_NORMAL) {
            buzzer_off();
        }
    }
}

IhmAlertState ihm_get_state() {
    return g_state;
}

IhmPage ihm_get_page() {
    return g_page;
}

void ihm_force_page(IhmPage page) {
    g_forcePage = true;
    g_forcedPage = page;
}

void ihm_release_forced_page() {
    g_forcePage = false;
}

void ihm_update(uint32_t now) {
    if (!g_initialized) return;

    update_page(now);
    update_leds();
    update_buzzer(now);
    render_page();
}