#ifndef PHYSICS_ENGINE_H
#define PHYSICS_ENGINE_H

#include "Config.h"
#include "CarData.h"
#include "Logging.h"

/**
 * @class PhysicsEngine
 * @brief Computes velocity, displacement, friction, and gravity ramp forces
 *        for active cars along the NeoPixel track length.
 */
class PhysicsEngine {
private:
    int _trackLength;
    uint8_t _maxLaps;
    int8_t _gravityMap[MAX_TRACK_LEDS];

    CarData _cars[TOTAL_PLAYERS] = {
        CarData(0), CarData(1), CarData(2), CarData(3)
    };

    uint8_t _trackLengthMeters;
    uint8_t _difficulty; // 0=Easy, 1=Medium, 2=Hard, 3=Very Hard
    bool _showHills;
    uint8_t _activePlayers;

    // Helper to calculate pixel index from distance in meters
    int mToLeds(float meters) const {
        return (int)(meters * LEDS_PER_METER);
    }

    /**
     * @brief Creates virtual hills (gravity maps) depending on track length and difficulty.
     */
    void generateTrackHills() {
        clearEnvironment();
        if (!_showHills) return;

        // Difficulty levels mapping: easy=2, medium=3, hard=4, very hard=5
        int8_t H = _difficulty + 2;

        #ifdef LOCAL_TEST_MODE
            // In local 29-LED test mode, place a single test hill at standard offsets
            setRamp(H, 15, 21, 27);
        #else
            // Place hills dynamically scaled by the configured LEDS_PER_METER density
            if (_trackLengthMeters == 5) {
                setRamp(H, mToLeds(1.5f), mToLeds(2.5f), mToLeds(3.5f));
            } else if (_trackLengthMeters == 10) {
                setRamp(H - 1, mToLeds(2.0f), mToLeds(3.0f), mToLeds(4.0f));
                setRamp(H,     mToLeds(6.0f), mToLeds(7.2f), mToLeds(8.4f));
            } else if (_trackLengthMeters == 15) {
                setRamp(H - 2, mToLeds(1.5f), mToLeds(2.5f), mToLeds(3.5f));
                setRamp(H - 1, mToLeds(5.5f), mToLeds(6.7f), mToLeds(7.9f));
                setRamp(H,     mToLeds(10.0f), mToLeds(11.5f), mToLeds(13.0f));
            }
        #endif
    }

public:
    PhysicsEngine() : 
        _maxLaps(5), _trackLengthMeters(5), _difficulty(1), _showHills(true), _activePlayers(4) {
        #ifdef LOCAL_TEST_MODE
            setTrackLength(TEST_TRACK_LEN);
        #else
            setTrackLength(mToLeds(_trackLengthMeters));
        #endif
        clearEnvironment();
        generateTrackHills();
    }

    void clearEnvironment() {
        for (int i = 0; i < MAX_TRACK_LEDS; i++) {
            _gravityMap[i] = 0; // 0 means perfectly neutral flat ground
        }
    }

    // Generates a virtual vector hill on your flat LED strip
    // H = steepness modifier, a = uphill start index, b = peak crest, c = downhill end index
    void setRamp(int8_t H, int a, int b, int c) {
        if (c >= _trackLength || a < 0) return; // Array bounds protection

        // Uphill segment calculation (negative gravity vector applied)
        float uphillSpan = b - a;
        for (int i = 0; i < uphillSpan; i++) {
            _gravityMap[a + i] = (int8_t)(0 - (i * (H / uphillSpan)));
        }

        // Downhill segment calculation (positive gravity vector applied)
        float downhillSpan = c - b;
        _gravityMap[b] = H; // Crest maximum kinetic potential push
        for (int i = 0; i <= downhillSpan; i++) {
            _gravityMap[b + i] = (int8_t)(H - (i * (H / downhillSpan)));
        }
    }

    void stepEngine() {
        for (uint8_t i = 0; i < _activePlayers; i++) {
            CarData& car = _cars[i];
            
            if (car.isFinished) {
                car.speed = 0.0f;
                continue;
            }

            int currentPixel = getCarTrackPixelIndex(i);
            
            // Extract baseline environmental gravity vector bias at target location
            float calculatedGravityForce = 0.0f;
            if (_showHills) {
                float gravityMultiplier = _difficulty * GRAVITY_K;
                calculatedGravityForce = _gravityMap[currentPixel] * gravityMultiplier;
            }

            // Apply slope mechanics derived from the terrain engine
            car.speed += calculatedGravityForce;

            // Apply ambient dynamic kinetic friction bleed
            car.speed -= car.speed * FRICTION_K;

            // Bound speed to prevent backward runaway states
            if (car.speed < -0.5f) car.speed = -0.5f;

            // Commit displacement step vector
            car.position += car.speed;
            if (car.position < 0.0f) car.position = 0.0f;

            // Apply speed penalty on approaching a hill at high speed
            int nextPixel = (int)car.position % _trackLength;
            if (_showHills && _gravityMap[currentPixel] == 0 && _gravityMap[nextPixel] < 0) {
                if (car.speed > HILL_PENALTY_SPEED_THRESHOLD) {
                    float oldSpeed = car.speed;
                    car.speed -= HILL_PENALTY_REDUCTION;
                    if (car.speed < 0.0f) car.speed = 0.0f;
                    SYS_LOG("[Penalty] Player %d hit the hill too fast! Speed reduced from %.2f to %.2f\n", 
                            i + 1, oldSpeed, car.speed);
                }
            }

            // Lap line cross assessment
            uint32_t currentTrackLap = (uint32_t)(car.position / _trackLength);
            if (currentTrackLap > car.currentLap) {
                car.currentLap = currentTrackLap;
                SYS_LOG("[Event] Player %d completed lap %d/%d!\n", i + 1, car.currentLap, _maxLaps);
                if (car.currentLap >= _maxLaps) {
                    car.isFinished = true;
                }
            }
        }
    }

    // Dispatches a manual driver pulse straight into a car instance
    void dispatchThrottle(uint8_t playerId) {
        if (playerId < TOTAL_PLAYERS) {
            CarData& car = _cars[playerId];
            if (!car.isFinished) {
                car.speed += ACCEL_PULSE;
                car.buttonPresses++;
            }
        }
    }

    void resetEngine() {
        for (uint8_t i = 0; i < TOTAL_PLAYERS; i++) {
            _cars[i].position = 0.0f;
            _cars[i].speed = 0.0f;
            _cars[i].buttonPresses = 0;
            _cars[i].currentLap = 0;
            _cars[i].isFinished = false;
        }
    }

    // Engine Configuration Modifiers
    void setTrackLength(int length) { _trackLength = length; }
    int getTrackLength() const { return _trackLength; }
    
    void setMaxLaps(uint8_t laps) { _maxLaps = laps; }
    uint8_t getMaxLaps() const { return _maxLaps; }
    int8_t getGravityValueAt(int index) const { return _gravityMap[index]; }

    void setTrackLengthMeters(uint8_t length) {
        _trackLengthMeters = (length == 5 || length == 10 || length == 15) ? length : 5;
        #ifdef LOCAL_TEST_MODE
            setTrackLength(TEST_TRACK_LEN);
        #else
            setTrackLength(mToLeds(_trackLengthMeters));
        #endif
        generateTrackHills();
    }
    uint8_t getTrackLengthMeters() const { return _trackLengthMeters; }

    void setDifficulty(uint8_t diff) {
        _difficulty = (diff <= 3) ? diff : 1;
        generateTrackHills();
    }
    uint8_t getDifficulty() const { return _difficulty; }

    void setShowHills(bool show) {
        _showHills = show;
        generateTrackHills();
    }
    bool getShowHills() const { return _showHills; }

    void setActivePlayers(uint8_t count) {
        _activePlayers = (count >= 1 && count <= TOTAL_PLAYERS) ? count : TOTAL_PLAYERS;
    }
    uint8_t getActivePlayers() const { return _activePlayers; }

    // Read-only object reference accessors for rendering and web hooks
    const CarData& getCarData(uint8_t playerId) const { return _cars[playerId]; }

    int getCarTrackPixelIndex(uint8_t playerId) const {
        if (playerId >= TOTAL_PLAYERS) return 0;
        return (int)_cars[playerId].position % _trackLength;
    }

    uint8_t getLeaderId() const {
        uint8_t leaderId = 0;
        float maxPos = -1.0f;
        for (uint8_t i = 0; i < TOTAL_PLAYERS; i++) {
            if (_cars[i].position > maxPos) {
                maxPos = _cars[i].position;
                leaderId = i;
            }
        }
        return leaderId;
    }

    bool isRaceFinished() const {
        for (uint8_t i = 0; i < TOTAL_PLAYERS; i++) {
            if (_cars[i].isFinished) {
                return true;
            }
        }
        return false;
    }
};

#endif // PHYSICS_ENGINE_H