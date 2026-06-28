#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

/**
 * @file Config.h
 * @brief Global system configurations, pin assignments, physics constants, and game states.
 */

// WiFi Access Point Settings
#define WIFI_AP_SSID        "ESP32-LEDRaceGrid"
#define WIFI_AP_PASS        "flagtoflag"

// System & Timing
#if CONFIG_FREERTOS_UNICORE || (portNUM_PROCESSORS == 1)
#define CORE_NETWORKING     0  // Fallback to Core 0 on single-core chips
#define CORE_GAME_ENGINE    0  // Fallback to Core 0 on single-core chips
#else
#define CORE_NETWORKING     0  // Core 0 handles Webserver & LCD UI
#define CORE_GAME_ENGINE    1  // Core 1 handles Physics & NeoPixel rendering
#endif

#define PHYSICS_TICK_RATE   30 // Run the physics loop at 30Hz
#define PHYSICS_PERIOD_MS   (1000 / PHYSICS_TICK_RATE)

// NeoPixel Track
#define TRACK_PIN           12
#define MAX_TRACK_LEDS      1000 // Increased safety ceiling to support high-density configurations

// Local Test Override Configuration
#define LOCAL_TEST_MODE          // Comment this out to run standard length * LEDS_PER_METER calculation

#ifdef LOCAL_TEST_MODE
#define TEST_TRACK_LEN   29   // For the 29-LED local test setup
#else
#define TEST_TRACK_LEN   300  // Default fallback on boot (e.g. 5m * 60 leds/m)
#endif

// Track Setup Config
#define LEDS_PER_METER      60   // Physical density of NeoPixels per meter of strip

// Player Inputs
#define TOTAL_PLAYERS       4
const uint8_t PLAYER_PINS[TOTAL_PLAYERS] = {14, 32, 15, 26};

// UI Buttons
#define BTN_UI_UP           5  // Tactical button for scrolling up
#define BTN_UI_SELECT       19 // Tactical button for item select
#define BTN_UI_DOWN         21 // Tactical button for scrolling down

#define UI_DEBOUNCE_MS      150
#define PLAYER_DEBOUNCE_MS  150

// Hardware Outputs
#define LCD_SDA             22
#define LCD_SCL             20
#define LCD_I2C_ADDR        0x27

#define RGB_RED_PIN         25
#define RGB_GREEN_PIN       4
#define RGB_BLUE_PIN        27

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
const float HILL_PENALTY_SPEED_THRESHOLD = 2.0f; // Speed threshold above which penalty applies
const float HILL_PENALTY_REDUCTION      = 0.5f; // Speed subtracted as penalty

// Game State Structures
enum GameState {
    STATE_STANDBY,
    STATE_COUNTDOWN,
    STATE_RACE_ACTIVE,
    STATE_CELEBRATION,
    STATE_MENU_CONFIG
};

struct PlayerInputEvent {
    uint8_t playerId;
    uint32_t timestamp;
};

#endif // CONFIG_H