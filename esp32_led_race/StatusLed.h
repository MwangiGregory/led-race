#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <Arduino.h>
#include "Config.h"
#include "PhysicsEngine.h"

/**
 * @class StatusLed
 * @brief Manages a physical RGB status LED (common-anode or common-cathode)
 *        to visually indicate game states and race progress.
 */
class StatusLed {
private:
    uint8_t _redPin;
    uint8_t _greenPin;
    uint8_t _bluePin;
    bool _activeLow;

public:
    StatusLed(uint8_t redPin = RGB_RED_PIN, uint8_t greenPin = RGB_GREEN_PIN, uint8_t bluePin = RGB_BLUE_PIN, bool activeLow = false)
        : _redPin(redPin), _greenPin(greenPin), _bluePin(bluePin), _activeLow(activeLow) {}

    /**
     * @brief Configure pins as outputs and turn the LED off.
     */
    void begin() {
        pinMode(_redPin, OUTPUT);
        pinMode(_greenPin, OUTPUT);
        pinMode(_bluePin, OUTPUT);
        off();
    }

    /**
     * @brief Set discrete RGB state.
     */
    void setColor(bool r, bool g, bool b) {
        digitalWrite(_redPin, _activeLow ? !r : r);
        digitalWrite(_greenPin, _activeLow ? !g : g);
        digitalWrite(_bluePin, _activeLow ? !b : b);
    }

    /**
     * @brief Turn off the RGB LED.
     */
    void off() {
        setColor(false, false, false);
    }

    /**
     * @brief Periodically called to handle state-specific colors and animation sequences.
     */
    void update(GameState state, uint32_t msTime, const PhysicsEngine& physics, uint32_t countdownElapsedMs = 0) {
        switch (state) {
            case STATE_STANDBY:
                // Solid Blue
                setColor(false, false, true);
                break;
            case STATE_COUNTDOWN:
                // Formula 1 style: flash Red for 3, 2, 1, then solid Green for GO!
                if (countdownElapsedMs < 3000) {
                    if ((countdownElapsedMs / 250) % 2 == 0) {
                        setColor(true, false, false);
                    } else {
                        off();
                    }
                } else {
                    setColor(false, true, false);
                }
                break;
            case STATE_RACE_ACTIVE:
                // Show the color of the current race leader
                {
                    uint8_t leaderId = physics.getLeaderId();
                    switch (leaderId) {
                        case 0: setColor(false, false, true); break;  // P1: Blue
                        case 1: setColor(true, false, false); break;  // P2: Red
                        case 2: setColor(false, true, false); break;  // P3: Green
                        case 3: setColor(true, true, false); break;   // P4: Yellow (Red + Green)
                        default: off(); break;
                    }
                }
                break;
            case STATE_CELEBRATION:
                // Cycle colors quickly (Rainbow flash)
                {
                    uint8_t cycle = (msTime / 150) % 6;
                    switch (cycle) {
                        case 0: setColor(true, false, false); break; // Red
                        case 1: setColor(true, true, false); break;  // Yellow
                        case 2: setColor(false, true, false); break; // Green
                        case 3: setColor(false, true, true); break;  // Cyan
                        case 4: setColor(false, false, true); break; // Blue
                        case 5: setColor(true, false, true); break;  // Magenta
                    }
                }
                break;
            case STATE_MENU_CONFIG:
                // Solid Magenta (Red + Blue)
                setColor(true, false, true);
                break;
            default:
                off();
                break;
        }
    }
};

#endif // STATUS_LED_H
