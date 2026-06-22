# API Reference & Code Documentation

This document provides a detailed breakdown of the classes, methods, functions, data structures, and multi-core task orchestration in the **Open LED Race v2** firmware.

---

## 📂 Codebase Overview

The codebase is structured under an object-oriented Model-View-Controller (MVC) architecture, utilizing a Singleton manager to coordinate states and resources across the ESP32's dual-core processor.

```
ebs_led_race/
├── ebs_led_race.ino      # Main setup and FreeRTOS task spawning (interrupt debouncing)
├── Config.h              # Global pin mappings, timing presets, and physics constants
├── Logging.h             # Standardized printf-styled logging macros (SYS_LOG)
├── CarData.h             # Structure tracking player telemetry metrics
├── PhysicsEngine.h       # Physics model (friction, hill tracking, displacement)
├── GameManager.h         # Singleton controller coordinating state, timing, NVS high scores
├── TaskManager.h/.cpp    # Core-pinned FreeRTOS task loops
├── TrackRenderer.h       # WS2812B NeoPixel drawing logic (Dirty Flag & quadratic hill gradients)
├── LcdMenu.h             # I2C LCD character display scrolling interface
├── UiInputModule.h       # Polled menu buttons with debounce & race abort control
├── StatusLed.h           # Physical RGB Status LED driver (F1 lights, leader colors)
└── WebServerModule.h     # Wi-Fi SoftAP and WebSocket telemetry dashboard
```

---

## ⚙️ 1. Configurations (`Config.h` & `Logging.h`)

Contains preprocessor constants, pin definitions, global data types, and constants.

### Key Definitions (`Config.h`)
*   `CORE_NETWORKING` (Core `0`): Runs the LCD UI, web server, and WebSocket telemetry.
*   `CORE_GAME_ENGINE` (Core `1`): Runs physics stepping and NeoPixel rendering.
*   `PHYSICS_TICK_RATE` (`30`): Hertz rating for the physics calculation thread.
*   `MAX_TRACK_LEDS` (`1000`): Maximum memory buffer allocation for the NeoPixel strip.
*   `LEDS_PER_METER` (`60`): Configuration macro allowing developers to specify the LED density of their strip.
*   `LOCAL_TEST_MODE`: Preprocessor macro to override calculation and force track length to `TEST_TRACK_LEN` (29) for testing.
*   `TEST_TRACK_LEN` (`29` or `300`): Default length depending on `LOCAL_TEST_MODE`.
*   `ACCEL_PULSE`: Selected player acceleration impulse (currently set to `ACCEL_PULSE_INSANE` which is `0.22f`). Also includes selectable presets: `VERY_LOW` (0.02f), `LOW` (0.04f), `MEDIUM` (0.08f), `HIGH` (0.12f), and `INSANE` (0.22f).
*   `HILL_PENALTY_SPEED_THRESHOLD` (`2.0f`): Speed threshold above which a vehicle receives a penalty.
*   `HILL_PENALTY_REDUCTION` (`0.5f`): Speed value subtracted when the penalty triggers.

### Logging Macros (`Logging.h`)
*   `SYS_LOG(...)`: Standard printf-styled logging wrapper for serial prints.
*   `SYS_LOG_THROTTLED(interval_ms, ...)`: An interval-checked logging macro that suppresses redundant console outputs.

### Global Data Types
*   `enum GameState`: Represents the system lifecycle state:
    *   `STATE_STANDBY`: Grid is ready; waiting for SELECT to start countdown.
    *   `STATE_COUNTDOWN`: Start sequence counting down (3, 2, 1, GO).
    *   `STATE_RACE_ACTIVE`: Active racing loop.
    *   `STATE_CELEBRATION`: Winner has crossed the finish line.
    *   `STATE_MENU_CONFIG`: Scrolling configuration settings menu.
*   `struct PlayerInputEvent`: Queue event structure:
    *   `uint8_t playerId`: Player identifier (`0` to `3`).
    *   `uint32_t timestamp`: FreeRTOS tick timestamp at button press.

---

## 🏎️ 2. Player Model (`CarData.h`)

Tracks variables for each player’s vehicle.

### Fields
*   `uint8_t id`: Player ID (`0` to `3`).
*   `float position`: Accumulated distance covered along the track.
*   `float speed`: Current velocity vector.
*   `uint32_t buttonPresses`: Total button click count (performance telemetry).
*   `uint8_t currentLap`: Current track lap count.
*   `bool isFinished`: Becomes `true` when `currentLap` reaches target laps.

---

## 📈 3. Physics Simulation (`PhysicsEngine.h`)

Calculates the displacement of vehicles based on momentum, friction, and gravity ramps.

### Methods
*   `void stepEngine()`: Updates speeds and positions for all *active* players. Applies gravity values, friction multipliers (`FRICTION_K = 0.015f`), and updates Lap metrics. Applies a speed penalty of `HILL_PENALTY_REDUCTION` (`0.5f`) when entering an uphill segment at a speed greater than `HILL_PENALTY_SPEED_THRESHOLD` (`2.0f`).
*   `void dispatchThrottle(uint8_t playerId)`: Adds a speed impulse (`ACCEL_PULSE`) to a player's car and increments their press count.
*   `void resetEngine()`: Restores all variables (positions, speeds, laps) to zero.
*   `void generateTrackHills()`: Clears the gravity mapping and generates ramps (1 ramp for 5m, 2 ramps for 10m, 3 ramps for 15m).
*   `void setRamp(int8_t H, int a, int b, int c)`: Generates linear slope gravity forces.
*   `int mToLeds(float meters) const`: Helper to calculate the pixel index from a distance in meters.
*   `bool isRaceFinished() const`: Returns `true` if any active player has `isFinished == true`.
*   `uint8_t getLeaderId() const`: Loops active players and returns the ID of the player with the highest track position.
*   `void setTrackLengthMeters(uint8_t length)`: Sets the physical length setting (`5m` -> 29 LEDs; `10m` -> 100 LEDs; `15m` -> 150 LEDs).
*   `void setDifficulty(uint8_t diff)`: Adjusts hill incline heights (`0`=EASY, `1`=MEDIUM, `2`=HARD, `3`=VERY HARD).
*   `void setShowHills(bool show)`: Toggles the physical hill gravity effects.

---

## 🎛️ 4. System Manager (`GameManager.h`)

A thread-safe **Singleton** class managing timing, high scores, and accessors.

### Methods
*   `static GameManager& getInstance()`: Accessor for the global singleton instance.
*   `void startCountdown()`: Resets winner variables, calls `resetGame()`, sets the start time timestamp (`millis()`), and changes the state to `STATE_COUNTDOWN`.
*   `void checkRaceFinished(const PhysicsEngine& physics)`: Compares active player progress. If a player wins, records `winnerTime` (in seconds), shifts state to `STATE_CELEBRATION`, and checks if a persistent high score was broken.
*   `float getHighScore() const`: Returns the best record time loaded from NVS.
*   `void setState(GameState state)`: Transition system state and outputs log details via `SYS_LOG`.

---

## 🎨 5. LED Rendering (`TrackRenderer.h`)

Translates physics variables into physical LED states using the `Adafruit_NeoPixel` library. Uses a **Dirty Flag** layout to prevent redundant updates.

### Methods
*   `void renderTrack(const PhysicsEngine& physics, GameState systemState)`:
    *   Checks if state, length, hill settings, or any player's position has changed.
    *   If changes are detected (`dirty == true`), it redrafts the environmental canvas and player layers and calls `_strip.show()`.
    *   Otherwise, it skips `show()`, saving CPU cycles and preventing signal jitter.
    *   Clears the strip automatically on transition to `STATE_MENU_CONFIG`.
*   `void drawEnvironment(const PhysicsEngine& physics)`: Renders background colors. If `Show Hills` is active, it draws Uphill segments in static, dimmed Magenta (`BG_UPHILL`) and Downhills in static, dimmed Cyan/Teal (`BG_DOWNHILL`). Otherwise, clears the track to flat black (`BG_FLAT`).
    *   Turns off all LEDs beyond `trackLength` up to `MAX_TRACK_LEDS` to prevent ghost pixels.
*   `void drawPlayer(uint8_t playerId, const PhysicsEngine& physics)`: Draws player cars. Player tails scale longer as they complete more laps (`tailLength = currentLap`).
*   `void runTheaterChaseCelebration(uint8_t winnerId, int trackLength)`: Plays a non-blocking theater chase animation cycling the winner's color in steps of 10 pixels every 50ms.

---

## 📺 6. LCD Scrolling UI (`LcdMenu.h`)

Drives the 16x4 LiquidCrystal I2C character screen.

### Methods
*   `void updateDisplayCanvas(const PhysicsEngine& physics, GameState globalState, int8_t currentMenuRow, uint32_t countdownElapsedMs)`:
    *   Clears LCD automatically on state transitions.
    *   `STATE_MENU_CONFIG`: Implements a **3-line scrolling window** to navigate through 6 setup parameters (Laps, Length, Difficulty, Show Hills, Players, Save & Exit).
    *   `STATE_COUNTDOWN`: Displays starting messages (`- 3 -`, `- 2 -`, `- 1 -`, `GO!`) synchronised to timer ticks.
    *   `STATE_RACE_ACTIVE`: Shows leaderboard stats (Laps/Speed) for active players. Inactive players are hidden.

---

## 🕹️ 7. Menu Navigation Input (`UiInputModule.h`)

Polls the Up, Down, and Select navigation buttons with software debounce filters.

### Methods
*   `GameState handleInputs(PhysicsEngine& physics, GameState currentState)`:
    *   Monitors button pin levels.
    *   Returns the next `GameState` based on input actions.
    *   If SELECT is pressed during `STATE_COUNTDOWN` or `STATE_RACE_ACTIVE`, it returns `STATE_STANDBY`, allowing the user to immediately abort the race.

---

## 🚨 8. Status LED Driver (`StatusLed.h`)

Drives the discrete common-cathode/anode RGB Status LED to show state indicators.

### Methods
*   `void update(GameState state, uint32_t msTime, const PhysicsEngine& physics, uint32_t countdownElapsedMs)`:
    *   `STATE_STANDBY`: Solid **Blue**.
    *   `STATE_MENU_CONFIG`: Solid **Magenta**.
    *   `STATE_COUNTDOWN`: Flashing **Red** during 3-2-1 sequence; solid **Green** at `GO!`.
    *   `STATE_RACE_ACTIVE`: Displays the solid color of the **current race leader** (P1: Red, P2: Blue, P3: Green, P4: Yellow).
    *   `STATE_CELEBRATION`: Triggers a rapid **rainbow color cycle**.

---

## 🌐 9. Wi-Fi AP & Telemetry Web Server (`WebServerModule.h`)

Runs a local SoftAP hotspot serving an interactive telemetry page and handling WebSockets.

### Methods
*   `void begin()`: Starts the `ESP32-LEDRaceGrid` hotspot and links WebSocket event bindings. Serves `INDEX_HTML` on `http://192.168.4.1`.
*   `void broadcastTelemetry(...)`: Serializes a rich JSON object containing state, active player count, leader ID, winner ID, win time, and NVS high score. Pushes this payload to browsers over WebSockets at a 10Hz rate.

---

## 🧵 10. Multi-Core Threads (`TaskManager.cpp` & `ebs_led_race.ino`)

Defines the FreeRTOS task loops running concurrently across the ESP32 cores, and player interrupt handlers.

### Player Button Interrupts (`ebs_led_race.ino`)
*   `handlePlayerButtonInterrupt`: Lightweight edge-triggered interrupt service routine (ISR). 
    *   Utilizes a `lastPlayerPressTime` timestamp array to enforce a **150ms hardware debounce window** inside the ISR context.
    *   Dispatches valid inputs to the `globalInputQueue` to prevent queue spam and rapid vehicle acceleration.

### Tasks (`TaskManager.cpp`)
*   `vPhysicsEngineTask` (Core `1`, Priority `tskIDLE_PRIORITY + 3`): Steps the physics calculations at 30Hz. Captures player click interrupt events from the queue.
*   `vTrackRenderTask` (Core `1`, Priority `tskIDLE_PRIORITY + 2`): Redraws the NeoPixels at ~50Hz.
    *   *Note*: The physics engine runs at a higher priority than the renderer to ensure highly responsive input processing.
*   `vAdminUiTask` (Core `0`, Priority `tskIDLE_PRIORITY + 1`): Handles buttons, updates the LCD, manages WebSocket cleanups, and broadcasts telemetry packets.
