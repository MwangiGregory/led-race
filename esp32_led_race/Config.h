#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Route to the appropriate board-specific configuration file based on target architecture
#if defined(CONFIG_IDF_TARGET_ESP32C3) || defined(ARDUINO_ESP32C3_DEV) || defined(ESP32C3)
#include "Config_ESP32_C3_SuperMini.h"
#else
#include "Config_ESP32_Feather.h"
#endif

// Game State Structures (Common to all architectures)
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