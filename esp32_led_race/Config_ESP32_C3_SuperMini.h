#ifndef CONFIG_ESP32_C3_SUPERMINI_H
#define CONFIG_ESP32_C3_SUPERMINI_H

#include <Arduino.h>

// WiFi Access Point Settings
#define WIFI_AP_SSID        "ESP32-LEDRaceGrid"
#define WIFI_AP_PASS        "flagtoflag"

// System & Timing
#define CORE_NETWORKING     0  // Single core: everything runs on Core 0
#define CORE_GAME_ENGINE    0  // Single core: everything runs on Core 0

#define PHYSICS_TICK_RATE   30 // Run the physics loop at 30Hz
#define PHYSICS_PERIOD_MS   (1000 / PHYSICS_TICK_RATE)

// NeoPixel Track
#define TRACK_PIN           10 // GPIO 10 on Super Mini
#define MAX_TRACK_LEDS      1000 // Safety ceiling

// Local Test Override Configuration
#define LOCAL_TEST_MODE          // Comment this out to run standard length * LEDS_PER_METER calculation

#ifdef LOCAL_TEST_MODE
#define TEST_TRACK_LEN   29   // For the 29-LED local test setup
#else
#define TEST_TRACK_LEN   300  // Default fallback on boot
#endif

// Track Setup Config
#define LEDS_PER_METER      60   // Physical density of NeoPixels per meter of strip

// Player Inputs (Mapped to edge-only pins, including GPIO 8 which is also the onboard LED)
#define TOTAL_PLAYERS       4
const uint8_t PLAYER_PINS[TOTAL_PLAYERS] = {0, 1, 2, 8}; // GPIO 0 (P1-RED), GPIO 1 (P2 - BLUE), GPIO 2 (P3 - GREEN ), GPIO 8 (P4 - YELLOW)

// UI Buttons
#define BTN_UI_UP           3  // GPIO 3 on Super Mini
#define BTN_UI_SELECT       9  // GPIO 9 on Super Mini
#define BTN_UI_DOWN         6  // GPIO 6 on Super Mini

#define UI_DEBOUNCE_MS      150
#define PLAYER_DEBOUNCE_MS  150

// Hardware Outputs
#define LCD_SDA             4  // GPIO 4 on Super Mini
#define LCD_SCL             5  // GPIO 5 on Super Mini
#define LCD_I2C_ADDR        0x27

#define RGB_RED_PIN         7  // GPIO 7 on Super Mini
#define RGB_GREEN_PIN       20 // GPIO 20 on Super Mini
#define RGB_BLUE_PIN        21 // GPIO 21 on Super Mini

// Physics Coefficients
// Speed increment presets
const float ACCEL_PULSE_VERY_LOW  = 0.02f;
const float ACCEL_PULSE_LOW       = 0.04f;
const float ACCEL_PULSE_MEDIUM    = 0.08f;
const float ACCEL_PULSE_HIGH      = 0.12f;
const float ACCEL_PULSE_INSANE    = 0.22f;

// Active acceleration selection (change this to select a different preset)
const float ACCEL_PULSE           = ACCEL_PULSE_INSANE;

const float FRICTION_K      = 0.015f;
const float GRAVITY_K       = 0.025f;

// Hill entry speed penalty configurations
const float HILL_PENALTY_SPEED_THRESHOLD = 2.0f;
const float HILL_PENALTY_REDUCTION      = 0.5f;

#endif // CONFIG_ESP32_C3_SUPERMINI_H
