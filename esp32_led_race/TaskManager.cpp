#include "TaskManager.h"
#include "Config.h"
#include "GameManager.h"
#include "Logging.h"

/**
 * @brief Thread routine for the physics engine task.
 *        Drains incoming ISR button event queue and performs numerical integration steps.
 */
void vPhysicsEngineTask(void* pvParameters) {
    TickType_t xLastWakeTime = xTaskGetTickCount();
    PlayerInputEvent inboundEvent;

    GameManager& gameEngine = GameManager::getInstance();
    QueueHandle_t queue = gameEngine.getInputQueue();
    PhysicsEngine& physics = gameEngine.getPhysicsEngine();

    SYS_LOG("[System Task] Physics Engine spawned on Core 1\n");

    while (true) {
        // Drain any pending button interrupt throttle events from the ISR queue
        while (xQueueReceive(queue, &inboundEvent, 0) == pdTRUE) {
            if (gameEngine.getState() == STATE_RACE_ACTIVE) {
                physics.dispatchThrottle(inboundEvent.playerId);
            } else if (gameEngine.getState() == STATE_STANDBY) {
                gameEngine.startCountdown();
            } else if (gameEngine.getState() == STATE_CELEBRATION) {
                gameEngine.setState(STATE_STANDBY);
                xQueueReset(queue);
                break;
            }
        }

        // Calculate next movement step and check if someone won the race
        if (gameEngine.getState() == STATE_RACE_ACTIVE) {
            physics.stepEngine();
            gameEngine.checkRaceFinished(physics);
        }

        // Run at precise periodic intervals (60Hz default)
        vTaskDelayUntil(&xLastWakeTime, pdMS_TO_TICKS(PHYSICS_PERIOD_MS));
    }
}

/**
 * @brief Thread routine for rendering animations and layouts on the NeoPixel LED strip.
 *        Executes render passes using internal dirty flags to minimize physical write operations.
 */
void vTrackRenderTask(void* pvParameters) {
    GameManager& gameEngine = GameManager::getInstance();
    PhysicsEngine& physics = gameEngine.getPhysicsEngine();
    TrackRenderer& renderer = gameEngine.getRenderer();
    
    SYS_LOG("[System Task] Track Animation Renderer spawned on Core 1\n");
    
    while (true) {
        // Redraw environment and players if changes occurred
        renderer.renderTrack(physics, gameEngine.getState());

        // Yield execution to match ~50fps redraw rate
        vTaskDelay(pdMS_TO_TICKS(20)); 
    }
}

/**
 * @brief Thread routine for web server telemetry broadcasts, LCD screens, and settings configuration.
 *        Runs on Core 0 to offload networking and slow I2C/UI tasks from the physics/rendering core.
 */
void vAdminUiTask(void* pvParameters) {
    GameManager& gameEngine = GameManager::getInstance();
    PhysicsEngine& physics = gameEngine.getPhysicsEngine();
    WebServerModule& webServer = gameEngine.getWebServer();
    LcdMenu& uiMenu = gameEngine.getUiMenu();
    UiInputModule& uiInput = gameEngine.getUiInput();
    StatusLed& statusLed = gameEngine.getStatusLed();
    
    SYS_LOG("[System Task] Admin UI Manager spawned on Core 0\n");

    gameEngine.setState(STATE_STANDBY);
    uiMenu.begin();
    
    while (true) {
        GameState currentState = gameEngine.getState();
        GameState nextState = uiInput.handleInputs(physics, currentState);
        
        // Push state transition if requested by UI buttons
        if (nextState != currentState) {
            if (nextState == STATE_COUNTDOWN) {
                gameEngine.startCountdown();
            } else {
                gameEngine.setState(nextState);
            }
        }

        // Manage transition from pre-race countdown to active racing
        if (gameEngine.getState() == STATE_COUNTDOWN) {
            static int lastLoggedSeconds = -1;
            uint32_t elapsed = millis() - gameEngine.getCountdownStartTime();
            int currentSeconds = elapsed / 1000;
            if (currentSeconds != lastLoggedSeconds) {
                lastLoggedSeconds = currentSeconds;
                if (currentSeconds == 0) SYS_LOG("[Event] Countdown: 3...\n");
                else if (currentSeconds == 1) SYS_LOG("[Event] Countdown: 2...\n");
                else if (currentSeconds == 2) SYS_LOG("[Event] Countdown: 1...\n");
                else if (currentSeconds == 3) SYS_LOG("[Event] GO!\n");
            }
            if (elapsed >= 4000) {
                lastLoggedSeconds = -1; // Reset for next countdown
                gameEngine.setState(STATE_RACE_ACTIVE);
                gameEngine.setRaceStartTime(millis());
            }
        }

        uint32_t countdownElapsed = 0;
        if (gameEngine.getState() == STATE_COUNTDOWN) {
            countdownElapsed = millis() - gameEngine.getCountdownStartTime();
        }

        // Periodically drop timed out WebSocket client sessions
        webServer.cleanup();

        // Send telemetry payload to active browser connections
        webServer.broadcastTelemetry(physics, gameEngine.getState(), gameEngine.getWinnerId(), gameEngine.getWinnerTime(), gameEngine.getHighScore(), countdownElapsed);

        // Redraw LCD canvas and update the physical RGB status LED
        uiMenu.updateDisplayCanvas(physics, gameEngine.getState(), uiInput.getCurrentMenuRow(), countdownElapsed);
        statusLed.update(gameEngine.getState(), millis(), physics, countdownElapsed);

        // Run at 20Hz frequency
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}