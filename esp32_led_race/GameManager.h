#ifndef GAME_MANAGER_H
#define GAME_MANAGER_H

#include "Config.h"
#include "PhysicsEngine.h"
#include "TrackRenderer.h"
#include "WebServerModule.h"
#include "LcdMenu.h"
#include "UiInputModule.h"
#include "StatusLed.h"
#include <Preferences.h>
#include "Logging.h"

/**
 * @class GameManager
 * @brief Singleton coordinator class managing the game states, countdown sequence,
 *        timing parameters, queue allocations, and persistent NVS high scores.
 */
class GameManager {
private:
    /**
     * @brief Private constructor to initialize the hardware modules, 
     *        input queues, and load persistent high scores.
     */
    GameManager() {
        playerInputQueue = xQueueCreate(20, sizeof(PlayerInputEvent));
        systemState = STATE_STANDBY;
        uiInput.begin();
        renderer.begin();
        webServer.begin();
        statusLed.begin();
        preferences.begin("ledrace", false);
        highScore = preferences.getFloat("highscore", 999.9f);
    }
    ~GameManager() = default;

    // Delete copy constructor and assignment operator since this is a singleton
    GameManager(const GameManager&) = delete;
    GameManager& operator=(const GameManager&) = delete;

    // Core shared memory structures
    volatile GameState systemState;
    QueueHandle_t playerInputQueue = nullptr;

    PhysicsEngine physics;
    TrackRenderer renderer;
    WebServerModule webServer;
    LcdMenu uiMenu;
    UiInputModule uiInput;
    StatusLed statusLed;

    Preferences preferences;
    uint32_t countdownStartTime = 0;
    uint8_t winnerPlayerId = 255;
    float winnerTime = 0.0f;
    float highScore = 999.9f;
    uint32_t raceStartTime = 0;
    uint32_t raceEndTime = 0;

public:
    /**
     * @brief Static singleton instance accessor.
     */
    static GameManager& getInstance() {
        static GameManager instance;
        return instance;
    }

    GameState getState() const { return systemState; }

    const char* getStateName(GameState state) const {
        switch (state) {
            case STATE_STANDBY:     return "STANDBY";
            case STATE_COUNTDOWN:   return "COUNTDOWN";
            case STATE_RACE_ACTIVE: return "RACE_ACTIVE";
            case STATE_CELEBRATION: return "CELEBRATION";
            case STATE_MENU_CONFIG: return "MENU_CONFIG";
            default:                return "UNKNOWN";
        }
    }

    void setState(GameState newState) { 
        if (systemState != newState) {
            SYS_LOG("[State Change] System transitioned to state: %s\n", getStateName(newState));
            systemState = newState; 
        }
    }

    uint32_t getCountdownStartTime() const { return countdownStartTime; }
    void startCountdown() {
        SYS_LOG("[Event] Starting Race countdown sequence...\n");
        countdownStartTime = millis();
        winnerPlayerId = 255;
        winnerTime = 0.0f;
        resetGame();
        setState(STATE_COUNTDOWN);
    }
    
    uint32_t getRaceStartTime() const { return raceStartTime; }
    void setRaceStartTime(uint32_t time) { raceStartTime = time; }

    uint8_t getWinnerId() const { return winnerPlayerId; }
    float getWinnerTime() const { return winnerTime; }
    float getHighScore() const { return highScore; }

    void checkRaceFinished(const PhysicsEngine& physics) {
        if (systemState == STATE_RACE_ACTIVE && physics.isRaceFinished()) {
            uint8_t winningId = 255;
            for (uint8_t i = 0; i < physics.getActivePlayers(); i++) {
                if (physics.getCarData(i).isFinished) {
                    winningId = i;
                    break;
                }
            }
            if (winningId != 255) {
                winnerPlayerId = winningId;
                winnerTime = (millis() - raceStartTime) / 1000.0f;
                setState(STATE_CELEBRATION);

                SYS_LOG("=================================================\n");
                SYS_LOG("[Event] Race Complete! WINNER: Player %u\n", winnerPlayerId + 1);
                SYS_LOG("[Stats] Winner Final Time: %.3fs\n", winnerTime);
                SYS_LOG("[Stats] Winner Throttle Presses: %u clicks\n", physics.getCarData(winnerPlayerId).buttonPresses);
                SYS_LOG("-------------------------------------------------\n");
                
                // Show other players final placement stats
                for (uint8_t i = 0; i < physics.getActivePlayers(); i++) {
                    if (i != winnerPlayerId) {
                        const CarData& car = physics.getCarData(i);
                        SYS_LOG("[Stats] Player %u: Lap %u, Position %.1f, Speed %.2f, Presses %u clicks\n",
                                      i + 1, car.currentLap, car.position, car.speed, car.buttonPresses);
                    }
                }
                
                if (winnerTime < highScore) {
                    highScore = winnerTime;
                    preferences.putFloat("highscore", highScore);
                    SYS_LOG("[Event] *** NEW TRACK HIGH SCORE RECORD: %.3fs ***\n", highScore);
                } else {
                    SYS_LOG("[Stats] Current Record to Beat: %.3fs\n", highScore);
                }
                SYS_LOG("=================================================\n");
            }
        }
    }

    void applyThrottle(uint8_t playerId) {
        physics.dispatchThrottle(playerId);
    }
    
    void resetGame() {
        physics.resetEngine();
    }
    
    QueueHandle_t getInputQueue() const { return playerInputQueue; }
    PhysicsEngine& getPhysicsEngine() { return physics; }
    TrackRenderer& getRenderer() { return renderer; }
    WebServerModule& getWebServer() { return webServer; }
    LcdMenu& getUiMenu() { return uiMenu; }
    UiInputModule& getUiInput() { return uiInput; }
    StatusLed& getStatusLed() { return statusLed; }
};

#endif // GAME_MANAGER_H