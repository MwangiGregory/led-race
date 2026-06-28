# 🔌 ESP32 Super Mini Hardware Setup Guide

The mapping **only uses the 16 pins exposed along the edges** of the board, completely avoiding the soldering pads on the underside.

## 🛠️ Component Connections & Wiring Details

### 1. WS2812B NeoPixel LED Strip
*   **Signal Level Shifter:** Since the LED strip operates on 5V logic and the ESP32-C3 outputs 3.3V, it is recommended to use a level shifter (e.g., **74AHCT125**) between `GPIO 10` and the strip's Data In (DIN) pin.

### 2. Player Throttle Buttons (x4)
*   **Active-Low Logic:** Connect tactile push buttons directly between their respective GPIO pins and Ground (GND).
*   **Internal Pull-Up**
*   **Strapping & Onboard LED Pins:** 
    *   **GPIO 2 (Player 3):** Strapping pin (must be high/floating during boot). Since normally open buttons leave the pin floating until pressed, this setup is safe to boot as long as the button is not held down during power-up.
    *   **GPIO 8 (Player 4):** Connected to the onboard blue status LED (Active-Low) and serves as a boot strapping pin. Using it for Player 4 is safe as long as the button is not held during boot. When the player clicks this button, the onboard blue LED will flash as visual tactile feedback!

### 3. Menu Navigation Buttons (x3)
*   **Active-Low Logic:** Connect UP, SELECT, and DOWN tactile switches between their respective GPIOs and GND.
*   **Strapping Pin Warning:** **GPIO 9** is the boot strapping pin (connected to the physical BOOT button on the board). Keep the SELECT button open during boot to ensure the chip boots into the game firmware instead of bootloader mode.

### 4. I2C Character LCD Display (16x4 or 20x4)
*   **I2C Bus Mappings:** Connect the display's SDA pin to `GPIO 4` and SCL pin to `GPIO 5`.
*   **Power:** Power the I2C backpack using the 5V rail.

### 5. Status RGB LED (Common Cathode)
*   **Wiring**: 
    *   Connect the **Common Cathode** pin to Ground (GND) through a single **330Ω resistor**. (Since the firmware only activates one color channel at any given time, a single resistor on the cathode is sufficient).
    *   Connect the Red, Green, and Blue anodes directly to their respective GPIO pins.
*   **Onboard LED Coexistence:** Since the onboard LED is on `GPIO 8` (Player 4 button), it will flash when Player 4 drives.

---

## 📌 Edge-Exposed Pinout Mapping

Ensure all components share a **common ground (GND)** connection.

| Silkscreen Label | ESP32-C3 GPIO | Game Component Connection | Description |
| :---: | :---: | :--- | :--- |
| **5V** | - | 5V Power Input / Output | System power input (VCC) |
| **GND** | - | Power Ground | Common Ground |
| **3V3** | - | 3.3V Regulator Output | 3.3V reference power |
| **0** | `GPIO 0` | **Player 1 Button (Blue)** | Throttle input (internal pullup) |
| **1** | `GPIO 1` | **Player 2 Button (Red)** | Throttle input (internal pullup) |
| **2** | `GPIO 2` | **Player 3 Button (Green)** | Throttle input (internal pullup) - *Strapping pin (keep open at boot)* |
| **3** | `GPIO 3` | **Menu UP Button** | Menu navigation input (internal pullup) |
| **4** | `GPIO 4` | **I2C LCD SDA** | I2C Serial Data line |
| **5** | `GPIO 5` | **I2C LCD SCL** | I2C Serial Clock line |
| **6** | `GPIO 6` | **Menu DOWN Button** | Menu navigation input (internal pullup) |
| **7** | `GPIO 7` | **RGB LED (Red Anode)** | Status LED Red control pin |
| **9** | `GPIO 9` | **Menu SELECT Button** | Select input (internal pullup) - *Boot strapping pin (keep open at boot)* |
| **10** | `GPIO 10` | **NeoPixel Strip DIN** | NeoPixel Data Output |
| **20** | `GPIO 20` | **RGB LED (Green Anode)** | Status LED Green control pin |
| **21** | `GPIO 21` | **RGB LED (Blue Anode)** | Status LED Blue control pin |
| **8** | `GPIO 8` | **Player 4 Button (Yellow)** | Throttle input (internal pullup) - *Onboard status LED & strapping pin (keep open at boot)* |

---

## ⚙️ Arduino IDE Configuration

When compiling the project for this board:
1. Open the Arduino IDE Board Manager.
2. Select **ESP32C3 Dev Module** (from the ESP32 board package).
3. Connect your board via USB-C. The ESP32-C3 features an internal USB-CDC controller, meaning it will expose a Serial Port directly without needing an external chip.
4. Update the pin definitions in `Config.h` to match the table above before compiling and uploading.

---

## 🌐 Accessing the Telemetry Web Dashboard
1. Connect to the Wi-Fi Hotspot:
   *   **SSID:** `ESP32-LEDRaceGrid`
   *   **Password:** `flagtoflag`
2. Open your browser and navigate to:
   *   **mDNS URL:** `http://ledrace.local` (or `http://192.168.4.1`)
