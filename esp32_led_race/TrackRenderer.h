#ifndef TRACK_RENDERER_H
#define TRACK_RENDERER_H

#include <Adafruit_NeoPixel.h>
#include "Config.h"
#include "PhysicsEngine.h"

/**
 * @class TrackRenderer
 * @brief Manages the WS2812B NeoPixel strip using Adafruit_NeoPixel library,
 *        implementing a Z-order overlay and Dirty Flag rendering logic.
 */
class TrackRenderer {
private:
    Adafruit_NeoPixel _strip;
    uint32_t _playerColors[TOTAL_PLAYERS];
    uint8_t  _zOrderShift;
    uint32_t _lastZOrderTick;

    // Render state tracking for Dirty Flag rendering
    GameState _lastState;
    int _lastCarPositions[TOTAL_PLAYERS];
    uint8_t _lastZOrderShift;
    bool _lastShowHills;
    int _lastTrackLength;
    uint8_t _lastActivePlayers;

    // Celebration animation states (theater chase)
    uint32_t _lastCelebrationFrameTick;
    uint8_t _celebrationFrame;

    // Palette Mapping Configurations (Dim color sets for backgrounds)
    uint32_t BG_FLAT;
    uint32_t BG_UPHILL;
    uint32_t BG_DOWNHILL;

    /**
     * @brief Paint the background colors representing the flat/hill terrain.
     */
    void drawEnvironment(const PhysicsEngine& physics) {
        int trackLength = physics.getTrackLength();
        bool showHills = physics.getShowHills();
        for (int i = 0; i < trackLength; i++) {
            int8_t slope = showHills ? physics.getGravityValueAt(i) : 0;
            if (slope < 0) {
                _strip.setPixelColor(i, BG_UPHILL);
            } else if (slope > 0) {
                _strip.setPixelColor(i, BG_DOWNHILL);
            } else {
                _strip.setPixelColor(i, BG_FLAT);
            }
        }
        // Turn off any physical LEDs on the strip beyond the active track length limit
        for (int i = trackLength; i < MAX_TRACK_LEDS; i++) {
            _strip.setPixelColor(i, _strip.Color(0, 0, 0));
        }
    }

    void drawPlayer(uint8_t playerId, const PhysicsEngine& physics) {
        int trackLength = physics.getTrackLength();
        const CarData& car = physics.getCarData(playerId);

        int tailLength = car.currentLap; // As lap count scales, trailing footprint grows
        int coreHeadPixel = physics.getCarTrackPixelIndex(playerId);

        // Extrude trailing footprint loops
        for (int t = 0; t <= tailLength; t++) {
            int renderingPixelIndex = coreHeadPixel - t;

            if (renderingPixelIndex < 0) renderingPixelIndex += trackLength;

            _strip.setPixelColor(renderingPixelIndex, _playerColors[playerId]);
        }
    }

    /**
     * @brief Plays the non-blocking theater chase animation frame for the winner.
     * @param winnerId The ID of the winning player.
     * @param trackLength The length of the track (number of pixels).
     */
    void runTheaterChaseCelebration(uint8_t winnerId, int trackLength) {
        uint32_t now = millis();
        int ledStep = 10;
        if (now - _lastCelebrationFrameTick >= 50) {
            _lastCelebrationFrameTick = now;
            _strip.clear();
            for (int c = _celebrationFrame; c < trackLength; c += ledStep) {
                _strip.setPixelColor(c, _playerColors[winnerId]);
            }
            _strip.show();
            _celebrationFrame = (_celebrationFrame + 1) % ledStep;
        }
    }

public:
    TrackRenderer() : 
        _strip(MAX_TRACK_LEDS, TRACK_PIN, NEO_GRB + NEO_KHZ800),
        _zOrderShift(0), _lastZOrderTick(0),
        _lastState(STATE_MENU_CONFIG), _lastZOrderShift(255),
        _lastShowHills(false), _lastTrackLength(-1), _lastActivePlayers(0),
        _lastCelebrationFrameTick(0), _celebrationFrame(0) {
        
        for (int i = 0; i < TOTAL_PLAYERS; i++) {
            _lastCarPositions[i] = -1;
        }
        
        // Define high-fidelity static player color vectors
        
        _playerColors[0] = _strip.Color(180, 0, 0);   // P1: Crimson Red
        _playerColors[1] = _strip.Color(0, 0, 180);   // P2: Vibrant Blue
        _playerColors[2] = _strip.Color(0, 150, 0);   // P3: Green
        _playerColors[3] = _strip.Color(130, 80, 0);  // P4: Amber Yellow

        // Environmental track canvas styles
        BG_FLAT     = _strip.Color(0, 0, 0);       // Clear dark canvas
        BG_UPHILL   = _strip.Color(20, 0, 20);     // Dim static Magenta for uphill ramps
        BG_DOWNHILL = _strip.Color(0, 20, 20);     // Dim static Cyan/Teal for downhill ramps
    }

    void begin() {
        _strip.begin();
        _strip.setBrightness(100); // Safety power draw dampening constraint
        clearStrip();
    }

    void clearStrip() {
        _strip.clear();
        _strip.show();
    }

    // Advanced Layered Canvas Builder (Z-Ordered Draw)
    void renderTrack(const PhysicsEngine& physics, GameState systemState) {

        if (systemState != _lastState && systemState == STATE_MENU_CONFIG) {
            _lastState = STATE_MENU_CONFIG;
            drawEnvironment(physics);
        }

        if (systemState == STATE_CELEBRATION) {
            uint8_t winnerId = 255;
            for (uint8_t i = 0; i < physics.getActivePlayers(); i++) {
                if (physics.getCarData(i).isFinished) {
                    winnerId = i;
                    break;
                }
            }
            if (winnerId != 255) {
                runTheaterChaseCelebration(winnerId, physics.getTrackLength());
            }
            return;
        }

        bool dirty = false;

        if (systemState != _lastState) {
            _lastState = systemState;
            dirty = true;
            _celebrationFrame = 0; // Reset animation counter when entering a new state
        }
    
        // during configuration
        if (physics.getTrackLength() != _lastTrackLength) {
            _lastTrackLength = physics.getTrackLength();
            dirty = true;
        }
        if (physics.getActivePlayers() != _lastActivePlayers) {
            _lastActivePlayers = physics.getActivePlayers();
            dirty = true;
        }
        if (physics.getShowHills() != _lastShowHills) {
            _lastShowHills = physics.getShowHills();
            dirty = true;
        }

        // during game play
        uint8_t activeCount = physics.getActivePlayers();
        for (uint8_t i = 0; i < activeCount; i++) {
            int pos = physics.getCarTrackPixelIndex(i);
            if (pos != _lastCarPositions[i]) {
                _lastCarPositions[i] = pos;
                dirty = true;
            }
        }

        if (systemState == STATE_RACE_ACTIVE || systemState == STATE_STANDBY || systemState == STATE_COUNTDOWN) {
            uint32_t now = millis();
            if (now - _lastZOrderTick >= 250) {
                _lastZOrderTick = now;
                _zOrderShift = (_zOrderShift + 1) % activeCount;
            }
            if (_zOrderShift != _lastZOrderShift) {
                _lastZOrderShift = _zOrderShift;
                dirty = true;
            }
        }

        if (dirty) {
            drawEnvironment(physics);
            if (systemState == STATE_RACE_ACTIVE || systemState == STATE_STANDBY || systemState == STATE_COUNTDOWN) {
                for (uint8_t layer = 0; layer < activeCount; layer++) {
                    uint8_t targetPlayerId = (layer + _zOrderShift) % activeCount;
                    drawPlayer(targetPlayerId, physics);
                }
            }
            _strip.show();
        }
    }
};

#endif // TRACK_RENDERER_H