# 🏎️ Open LED Race v2

## 🔌 Hardware Specifications & Wiring Guide

Below is the detailed specification of the components used, how they are wired up, and their pin mappings.

### 1. Component Specifications & Wiring Details

*   **Adafruit esp32 feather v2(hosts an esp32-s3 mcu)**
*   **WS2812B NeoPixel LED Strip**
*   **3 Buttons tactile Player Throttle Buttons**:
    *   **Wiring**: No external resistors are required.
*   **Tactile Menu Navigation Buttons (UP, SELECT, DOWN)**:
        **Wiring**: No external resistors are required.
*   **I2C Character LCD Display (20x4)**:
    *   **Wiring**: Connect SDA to `GPIO 22` and SCL to `GPIO 20`. Power the LCD module (with standard I2C backpack) from the 5V rail.
*   **Status RGB LED (Common Cathode)**:
    *   **Wiring**:
        *   **Common Cathode Pin**: Connected directly to Ground (GND) through a 330ohm resistor. Since the firmware only always turns on one led at a time.
        *   **Red Anode Pin**: Connected to `GPIO 25`
        *   **Green Anode Pin**: Connected to `GPIO 4`
        *   **Blue Anode Pin**: Connected to `GPIO 27`
---

### 2. Pinout Mappings

Ensure all components share a **common ground (GND)** connection.

| Component | Device Connection | ESP32 GPIO Pin | Description |
| :--- | :--- | :--- | :--- |
| **NeoPixel Strip** | DIN (Data In) | `GPIO 12` | Data output to 74AHCT125 level shifter |
| **Player 1 Throttle** | BUTTON 1 | `GPIO 14` | Input (internal pullup) for Blue Player |
| **Player 2 Throttle** | BUTTON 2 | `GPIO 32` | Input (internal pullup) for Red Player |
| **Player 3 Throttle** | BUTTON 3 | `GPIO 15` | Input (internal pullup) for Green Player |
| **Player 4 Throttle** | BUTTON 4 | `GPIO 26` | Input (internal pullup) for Yellow Player |
| **Menu UP Button** | UP | `GPIO 5` | Input (internal pullup) for menu scrolling |
| **Menu SELECT Button**| SELECT | `GPIO 19` | Input (internal pullup) for menu select |
| **Menu DOWN Button**  | DOWN | `GPIO 21` | Input (internal pullup) for menu scrolling |
| **I2C LCD Display** | SDA | `GPIO 22` | I2C Serial Data line |
| **I2C LCD Display** | SCL | `GPIO 20` | I2C Serial Clock line |
| **RGB Status LED** | RED Pin | `GPIO 25` | Output red channel |
| **RGB Status LED** | GREEN Pin | `GPIO 4` | Output green channel |
| **RGB Status LED** | BLUE Pin | `GPIO 27` | Output blue channel |

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
