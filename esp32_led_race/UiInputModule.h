#ifndef UI_INPUT_MODULE_H
#define UI_INPUT_MODULE_H

#include <Arduino.h>
#include "Config.h"
#include "PhysicsEngine.h"
#include "Logging.h"

/**
 * @class UiInputModule
 * @brief Handles menu navigation inputs by polling physical buttons
 *        and returning requested game state transitions.
 */
class UiInputModule {
private:
    uint32_t _lastButtonActionTime;
    bool _prevUp;
    bool _prevDown;
    bool _prevSelect;
    int8_t _currentMenuRow;

    static const uint8_t TOTAL_MENU_ITEMS = 6; 

    /**
     * @brief Debounce button presses to prevent double execution.
     */
    bool _isDebounced() {
        uint32_t now = millis();
        if (now - _lastButtonActionTime >= UI_DEBOUNCE_MS) {
            _lastButtonActionTime = now;
            return true;
        }
        return false;
    }

public:
    UiInputModule() : _lastButtonActionTime(0), _prevUp(false), _prevDown(false), 
                      _prevSelect(false), _currentMenuRow(0) {}

    /**
     * @brief Initialize menu button pins with internal pullup resistors.
     */
    void begin() {
        pinMode(BTN_UI_UP, INPUT_PULLUP);
        pinMode(BTN_UI_DOWN, INPUT_PULLUP);
        pinMode(BTN_UI_SELECT, INPUT_PULLUP);
    }

    /**
     * @brief Returns the index of the currently highlighted menu setting row.
     */
    int8_t getCurrentMenuRow() const {
        return _currentMenuRow;
    }

    /**
     * @brief Polls menu buttons and returns the next GameState based on actions.
     * @return GameState The new state, or currentState if no transitions occurred.
     */
    GameState handleInputs(PhysicsEngine& physics, GameState currentState) {
        bool currentUp     = (digitalRead(BTN_UI_UP) == LOW);
        bool currentDown   = (digitalRead(BTN_UI_DOWN) == LOW);
        bool currentSelect = (digitalRead(BTN_UI_SELECT) == LOW);

        bool pressedUp     = currentUp && !_prevUp;
        bool pressedDown   = currentDown && !_prevDown;
        bool pressedSelect = currentSelect && !_prevSelect;

        _prevUp = currentUp;
        _prevDown = currentDown;
        _prevSelect = currentSelect;

        if (!pressedUp && !pressedDown && !pressedSelect) return currentState; 
        if (!_isDebounced()) return currentState;                              

        if (currentState == STATE_MENU_CONFIG) {
            if (pressedUp) {
                _currentMenuRow--;
                if (_currentMenuRow < 0) _currentMenuRow = TOTAL_MENU_ITEMS - 1;
            } 
            else if (pressedDown) {
                _currentMenuRow = (_currentMenuRow + 1) % TOTAL_MENU_ITEMS;
            } 
            else if (pressedSelect) {
                switch (_currentMenuRow) {
                    case 0: 
                        physics.setMaxLaps(physics.getMaxLaps() >= 30 ? 5 : physics.getMaxLaps() + 5);
                        SYS_LOG("[Event] UI Config - Laps set to: %u\n", physics.getMaxLaps());
                        break;
                    case 1: 
                        physics.setTrackLengthMeters(physics.getTrackLengthMeters() >= 15 ? 5 : physics.getTrackLengthMeters() + 5);
                        SYS_LOG("[Event] UI Config - Track length set to: %um (%d LEDs)\n", physics.getTrackLengthMeters(), physics.getTrackLength());
                        break;
                    case 2: 
                        physics.setDifficulty((physics.getDifficulty() + 1) % 4);
                        {
                            const char* diffStr = "EASY";
                            if (physics.getDifficulty() == 1) diffStr = "MEDIUM";
                            else if (physics.getDifficulty() == 2) diffStr = "HARD";
                            else if (physics.getDifficulty() == 3) diffStr = "VERY HARD";
                            SYS_LOG("[Event] UI Config - Difficulty set to: %s\n", diffStr);
                        }
                        break;
                    case 3:
                        physics.setShowHills(!physics.getShowHills());
                        SYS_LOG("[Event] UI Config - Hills enabled: %s\n", physics.getShowHills() ? "YES" : "NO");
                        break;
                    case 4:
                        physics.setActivePlayers(physics.getActivePlayers() >= 4 ? 1 : physics.getActivePlayers() + 1);
                        SYS_LOG("[Event] UI Config - Active players set to: %u\n", physics.getActivePlayers());
                        break;
                    case 5: 
                        SYS_LOG("[Event] UI Config - Exited menu configurations.\n");
                        return STATE_STANDBY;
                }
            }
        } 
        else if (currentState == STATE_STANDBY) {
            if (pressedSelect) {
                return STATE_COUNTDOWN;
            } else if (pressedUp || pressedDown) {
                _currentMenuRow = 0;
                return STATE_MENU_CONFIG;
            }
        }
        else if (currentState == STATE_CELEBRATION) {
            if (pressedSelect) {
                return STATE_STANDBY;
            }
        }
        else if (currentState == STATE_RACE_ACTIVE || currentState == STATE_COUNTDOWN) {
            if (pressedSelect) {
                return STATE_STANDBY;
            }
        }

        return currentState;
    }
};

#endif // UI_INPUT_MODULE_H