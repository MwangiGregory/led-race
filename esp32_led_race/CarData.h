#ifndef CAR_DATA_H
#define CAR_DATA_H

#include <Arduino.h>

/**
 * @struct CarData
 * @brief Represents state and metrics for an individual player's car.
 */
struct CarData {
    uint8_t  id;            ///< Player ID identifier (0 to 3)
    float    position;      ///< Current absolute pixel position on track
    float    speed;         ///< Velocity vector of the car
    uint32_t buttonPresses; ///< Total button presses recorded (throttle commands)
    uint8_t  currentLap;    ///< Current lap index (0-based)
    bool     isFinished;    ///< Has the car crossed the finish line?

    /**
     * @brief Constructor initializing the car with a given player ID and zeroed metrics.
     */
    CarData(uint8_t initId) : 
        id(initId), position(0.0f), speed(0.0f), 
        buttonPresses(0), currentLap(0), isFinished(false) {}
};

#endif // CAR_DATA_H