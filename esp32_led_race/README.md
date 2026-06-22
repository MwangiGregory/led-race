# 🏎️ Open LED Race v2

Open LED Race v2 is an interactive physical slot-car style racing game powered by an ESP32 microcontroller, addressable WS2812B NeoPixel strips, a character LCD, and a real-time web telemetry dashboard. 

Players accelerate their virtual "cars" (represented by colored LED segments with lap-expanding tails) by clicking physical throttle buttons. The system runs real-time physics calculations (friction, drag, and slope gravity) and streams telemetry metrics to browser dashboards over WebSockets.

---

## 📸 Media & Interface Showcases

Here are visual guides to the system interfaces. Placeholders are provided below to link screenshots and video demos directly:

### 1. Web Telemetry Dashboard
This responsive dashboard shows real-time progress bars, speeds, current laps, high score lap records, a leader indicator crown (`👑`), and a victory trophy screen (`🏆`).
![Telemetry Web Dashboard Interface](docs/images/dashboard_screenshot.png)
*(Save a screenshot of the dashboard at `docs/images/dashboard_screenshot.png` to display it here)*

### 2. LCD & Button Menu Operation
Watch a video demonstration showing how to navigate the 6-item scroll settings menu (Laps, Track Length, Difficulty, Show Hills, Players, Save & Exit) and trigger the countdown start using the physical control buttons:
![LCD Menu Navigation Video](docs/videos/lcd_demo.mp4)
*(Save a short demo video of your LCD setup at `docs/videos/lcd_demo.mp4` to play it here)*

---

## 🛠️ Hardware Requirements & Pinout

### Mappings (Adafruit Feather ESP32 V2 / Standard ESP32)
Ensure all devices share a common ground reference.

| Component | Device Pin | ESP32 GPIO | Description |
| :--- | :--- | :--- | :--- |
| **NeoPixel Strip** | DIN (Data In) | `GPIO 12` | Controls up to 150 WS2812B LEDs (Default 29 for local test) |
| **Player 1 Throttle** | BUTTON 1 | `GPIO 14` | Edge-triggered pullup button for Player 1 (Red) |
| **Player 2 Throttle** | BUTTON 2 | `GPIO 32` | Edge-triggered pullup button for Player 2 (Blue) |
| **Player 3 Throttle** | BUTTON 3 | `GPIO 15` | Edge-triggered pullup button for Player 3 (Green) |
| **Player 4 Throttle** | BUTTON 4 | `GPIO 26` | Edge-triggered pullup button for Player 4 (Yellow) |
| **Menu Navigation UP** | UP | `GPIO 5` | Polled pullup menu navigation button |
| **Menu Select** | SELECT | `GPIO 19` | Polled pullup menu item select button |
| **Menu Navigation DOWN** | DOWN | `GPIO 21` | Polled pullup menu navigation button |
| **I2C LCD Display** | SDA | `GPIO 22` | Data line for I2C LCD character screen |
| **I2C LCD Display** | SCL | `GPIO 20` | Clock line for I2C LCD character screen |
| **Status RGB LED** | RED | `GPIO 25` | Output red pin for status indicators |
| **Status RGB LED** | GREEN | `GPIO 4` | Output green pin for status indicators |
| **Status RGB LED** | BLUE | `GPIO 27` | Output blue pin for status indicators |

> [!IMPORTANT]
> **Electrical Safety Notice:**
> WS2812B LEDs operate on a 5V logic signal, whereas the ESP32 outputs 3.3V. It is highly recommended to use a **74AHCT125 High-Speed Level Shifter** to convert the 3.3V data signal of `GPIO 12` to 5.0V. Connecting 3.3V directly to the strip can cause random white flashes or signal glitches.

---

## ⚙️ Core Game Settings

The scrolling settings menu on the LCD character screen allows you to configure:
1.  **Laps**: Choice of 5, 10, 15, 20, 25, or 30 laps to win.
2.  **Track Length**: Set to `5m` (runs on 29 LEDs locally), `10m` (100 LEDs), or `15m` (150 LEDs).
3.  **Difficulty**: Toggle `EASY`, `MEDIUM`, `HARD`, or `VERY HARD` to adjust slope gravity steepness.
4.  **Show Hills**: Toggle `YES` or `NO` to display/hide color-coded incline sections (Quadratic Magenta gradient for uphill, Quadratic Cyan/Teal gradient for downhill) and enable/disable physics calculations for gravity.
5.  **Players**: Select `1` to `4` active players.

---

## 🧠 Software Architecture

The firmware utilizes FreeRTOS to allocate processing tasks across the ESP32's dual-core processor, preventing web server activities from interrupting critical timing:

*   **Core 1 (`CORE_GAME_ENGINE`)**:
    *   `vPhysicsEngineTask` (Priority `tskIDLE_PRIORITY + 3`): Updates positions, speeds, laps, and friction at 30Hz. It processes queue-dispatched interrupt clicks (debounced at 150ms).
    *   `vTrackRenderTask` (Priority `tskIDLE_PRIORITY + 2`): Redraws the NeoPixels at 50Hz. It uses **Dirty Flag rendering** to only call `_strip.show()` when changes occur.
*   **Core 0 (`CORE_NETWORKING`)**:
    *   `vAdminUiTask` (Priority `tskIDLE_PRIORITY + 1`): Polls settings buttons, updates the LCD, handles WebSocket cleanups, and broadcasts telemetry packets.

---

## 🚀 Getting Started

### 1. Compile & Upload
The workspace is configured for the VS Code Arduino extension (`.vscode/arduino.json`).
1. Open the project folder in VS Code.
2. Select your sketch `ebs_led_race.ino` and connect the ESP32 to your USB port (e.g. `/dev/ttyUSB0` on Linux).
3. Verify and compile the code, then upload it to your board.

### 2. Connect to the Telemetry Dashboard
1. On boot, the ESP32 hosts a Wi-Fi Access Point:
    *   **SSID**: `ESP32-LEDRaceGrid`
    *   **Password**: `flagtoflag`
2. Connect to this Wi-Fi network from your phone, tablet, or laptop.
3. Open a web browser and navigate to `http://192.168.4.1`.
4. The dashboard will load and display live settings updates in real-time as you scroll the LCD menu!

---

## 🛠️ Code Reference
For developers looking to extend features, see the comprehensive [API reference manual](docs/api_documentation.md) located under the `docs` folder.
