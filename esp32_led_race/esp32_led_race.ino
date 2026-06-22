/**
 * @file ebs_led_race.ino
 * @brief Main entry point for the ESP32 Open LED Race game controller.
 *        Initializes hardware, configures player throttle interrupts,
 *        and spawns FreeRTOS tasks distributed across both processing cores.
 */

#include <Arduino.h>
#include "Config.h"
#include "GameManager.h"
#include "TaskManager.h"
#include "Logging.h"

// Cached queue handle to prevent singleton accessor resolution in ISR context
static QueueHandle_t globalInputQueue = nullptr;

// Track the last tick count a button press was debounced for each player
static volatile uint32_t lastPlayerPressTime[TOTAL_PLAYERS] = {0, 0, 0, 0};

/**
 * @brief Interrupt Service Routine (ISR) triggered by player button presses (falling edge).
 *        Identifies the player, applies debouncing, and dispatches a throttle event.
 *        Note: Must be kept extremely lightweight; no Serial printing or flash-based calls.
 */
void IRAM_ATTR handlePlayerButtonInterrupt(void* arg) {
    uint32_t pinNumber = (uint32_t)arg;
    uint8_t targetPlayerId = 255;

    for (uint8_t i = 0; i < TOTAL_PLAYERS; i++) {
        if (PLAYER_PINS[i] == pinNumber) {
            targetPlayerId = i;
            break;
        }
    }

    if (targetPlayerId != 255 && globalInputQueue != nullptr) {

        uint32_t now = xTaskGetTickCountFromISR();
        /**
         * Debounce Mechanism:
         * 1. The first falling edge triggers immediately, updates the timestamp, and queues the press.
         * 2. Successive interrupt triggers (bounces) occurring within 150ms of the first trigger 
         *    are ignored and do not update the timestamp.
         * 3. Once 150ms passes, the system is ready to accept the first falling edge of the next press.
         */
        if (now - lastPlayerPressTime[targetPlayerId] >= pdMS_TO_TICKS(150)) {
            lastPlayerPressTime[targetPlayerId] = now;

            PlayerInputEvent event;
            event.playerId = targetPlayerId;
            event.timestamp = now;
            
            // Push event directly to the cached queue handle
            xQueueSendFromISR(globalInputQueue, &event, NULL);
        }
    }
}

/**
 * @brief Main system initialization. Runs once on boot.
 */
void setup() {
    Serial.begin(115200);
    while(!Serial);
    SYS_LOG("\n=== OPEN LED RACE V2: SYSTEM READY ===\n");

    // Instantiate central GameManager singleton instance and cache input queue
    GameManager& gameEngine = GameManager::getInstance();
    globalInputQueue = gameEngine.getInputQueue();
    if (globalInputQueue == NULL) {
        SYS_LOG("Fatal Error: Queue creation fault.\n");
        while(1);
    }

    // Configure GPIO pull-ups and attach edge-triggered interrupts for player inputs
    for (uint8_t i = 0; i < TOTAL_PLAYERS; i++) {
        pinMode(PLAYER_PINS[i], INPUT_PULLUP);

        attachInterruptArg(
            digitalPinToInterrupt(PLAYER_PINS[i]),
            handlePlayerButtonInterrupt,
            (void*)(uint32_t)PLAYER_PINS[i],
            FALLING
        );
    }

    // Physics and rendering run on Core 1; Web server/UI runs on Core 0.
    xTaskCreatePinnedToCore(
        vPhysicsEngineTask, 
        "PhysicsEngine", 
        4096, 
        NULL, 
        tskIDLE_PRIORITY + 3, 
        NULL, 
        CORE_GAME_ENGINE);

    xTaskCreatePinnedToCore(
        vTrackRenderTask,   
        "TrackRenderer", 
        4096, 
        NULL, 
        tskIDLE_PRIORITY + 2, 
        NULL, 
        CORE_GAME_ENGINE);

    xTaskCreatePinnedToCore(
        vAdminUiTask,       
        "AdminUI",       
        8192, 
        NULL, 
        tskIDLE_PRIORITY + 1, 
        NULL, 
        CORE_NETWORKING);


    SYS_LOG("[System] All background processing modules are active.\n");
}

/**
 * @brief Standard Arduino execution loop. Not used since work is handled by FreeRTOS tasks.
 */
void loop() {
    // Delete
    vTaskDelete(NULL);
}