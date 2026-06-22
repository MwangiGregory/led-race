#ifndef LCD_MENU_H
#define LCD_MENU_H

#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "Config.h"
#include "PhysicsEngine.h"

/**
 * @class LcdMenu
 * @brief Manages the 16x4 character I2C LCD screen, rendering the booting animation,
 *        grid standby details, scrolling settings menu, and live race leaderboards.
 */
class LcdMenu {
private:
    LiquidCrystal_I2C _lcd;
    GameState _lastRenderedState;

public:
    // Initialize with a different state to ensure it clears when first moving to STANDBY
    LcdMenu() : _lcd(LCD_I2C_ADDR, 16, 4), _lastRenderedState(STATE_MENU_CONFIG) {}

    /**
     * @brief Initialize I2C communication and LCD hardware registers.
     */
    void begin() {
        Wire.begin(LCD_SDA, LCD_SCL);
        _lcd.init();
        _lcd.backlight();
        renderStaticBootScreen();
    }

    /**
     * @brief Render initial startup message on LCD.
     */
    void renderStaticBootScreen() {
        _lcd.clear();
        _lcd.setCursor(0, 0); _lcd.print("== LED RACE V2 ==");
        _lcd.setCursor(0, 1); _lcd.print("System Booting...");
        _lcd.setCursor(0, 3); _lcd.print("Status: Core OK ");
    }

    /**
     * @brief Update the LCD canvas depending on the current global State and settings.
     */
    void updateDisplayCanvas(const PhysicsEngine& physics, GameState globalState, int8_t currentMenuRow, uint32_t countdownElapsedMs = 0) {
        // Automatically clear display hardware on state changes
        if (globalState != _lastRenderedState) {
            _lcd.clear();
            _lastRenderedState = globalState;
        }

        switch (globalState) {
            case STATE_MENU_CONFIG:
                _lcd.setCursor(0, 0); _lcd.print("--- SETTINGS ---");
                {
                    int startItem = 0;
                    if (currentMenuRow >= 3) {
                        startItem = currentMenuRow - 2;
                    }
                    for (int i = 0; i < 3; i++) {
                        int itemIdx = startItem + i;
                        _lcd.setCursor(0, i + 1);
                        bool selected = (currentMenuRow == itemIdx);
                        _lcd.print(selected ? ">" : " ");
                        
                        switch (itemIdx) {
                            case 0:
                                _lcd.printf("Laps: %-8d", physics.getMaxLaps());
                                break;
                            case 1:
                                _lcd.printf("Length: %-6dm", physics.getTrackLengthMeters());
                                break;
                            case 2:
                                _lcd.print("Diff: ");
                                switch (physics.getDifficulty()) {
                                    case 0: _lcd.print("EASY     "); break;
                                    case 1: _lcd.print("MEDIUM   "); break;
                                    case 2: _lcd.print("HARD     "); break;
                                    case 3: _lcd.print("VERY HARD"); break;
                                }
                                break;
                            case 3:
                                _lcd.printf("Hills: %-7s", physics.getShowHills() ? "YES" : "NO");
                                break;
                            case 4:
                                _lcd.printf("Players: %-5d", physics.getActivePlayers());
                                break;
                            case 5:
                                _lcd.print("[SAVE & EXIT]  ");
                                break;
                        }
                    }
                }
                break;

            case STATE_STANDBY:
                _lcd.setCursor(0, 0); _lcd.print("== GRID READY ==");
                _lcd.setCursor(0, 1); _lcd.print("Press [SELECT]  ");
                _lcd.setCursor(0, 2); _lcd.print("to launch race.. ");
                _lcd.setCursor(0, 3);
                _lcd.printf("Laps:%-2d Plyrs:%-2d", physics.getMaxLaps(), physics.getActivePlayers());
                break;

            case STATE_COUNTDOWN:
                _lcd.setCursor(0, 0); _lcd.print("== GET READY! ==");
                _lcd.setCursor(0, 2);
                if (countdownElapsedMs < 1000) {
                    _lcd.print("     - 3 -      ");
                } else if (countdownElapsedMs < 2000) {
                    _lcd.print("     - 2 -      ");
                } else if (countdownElapsedMs < 3000) {
                    _lcd.print("     - 1 -      ");
                } else {
                    _lcd.print("    === GO! === ");
                }
                break;

            case STATE_RACE_ACTIVE:
                // Active Telemetry Monitor HUD
                _lcd.setCursor(0, 0); _lcd.print("LIVE LEADERBOARD");
                {
                    uint8_t activeCount = physics.getActivePlayers();
                    for (uint8_t i = 0; i < 3; i++) {
                        _lcd.setCursor(0, i + 1);
                        if (i < activeCount) {
                            const CarData& car = physics.getCarData(i);
                            _lcd.printf("P%d L:%d Spd:%.1f  ", i+1, car.currentLap, car.speed);
                        } else {
                            _lcd.print("                ");
                        }
                    }
                }
                break;

            case STATE_CELEBRATION:
                _lcd.setCursor(0, 1); _lcd.print("  RACE FINISHED! ");
                _lcd.setCursor(0, 2); _lcd.print(" Check Dashboard ");
                break;
        }
    }
};

#endif // LCD_MENU_H