# Smart-Plant-Monitor
A monitor built by a ESP32 that uses WiFi connection to showcase soil moisture, humidity, and temperature on a webpage and LED screen.
# 🌱 Smart Plant Monitor (ESP32)

A WiFi-connected plant monitor built on an ESP32 that tracks soil moisture, temperature, and humidity, displays them on a small OLED screen, and serves a live web page you can check from your phone.

## Features

- Capacitive soil moisture sensing (immune to corrosion, unlike resistive probes)
- Temperature and humidity via DHT11
- Live readout on a 0.96" SSD1306 OLED
- Built-in web server — open the ESP32's IP address on any phone/laptop on the same WiFi to see live readings, refreshed every 3 seconds
- No app install required — it's just a web page

## Hardware

| Part | Notes |
|---|---|
| ESP32 Dev Module (30-pin, ESP-WROOM-32) | Classic DevKit layout |
| Capacitive soil moisture sensor | v1.2 |
| DHT11 temperature/humidity sensor | |
| 0.96" SSD1306 OLED (I2C, 128x64) | Address `0x3C` |
| Full-size breadboard (830 tie-points) | A half-size board is too small for the ESP32 plus sensors |
| Jumper wires (male-to-male) | |

## Wiring

All parts share the ESP32's 3.3V and GND.

| Part | Pin | ESP32 Pin |
|---|---|---|
| OLED | VCC | 3V3 |
| OLED | GND | GND |
| OLED | SDA | GPIO 21 |
| OLED | SCL | GPIO 22 |
| DHT11 | VCC (+) | 3V3 |
| DHT11 | GND (-) | GND |
| DHT11 | DATA (S) | GPIO 4 |
| Soil sensor | VCC (red) | 3V3 |
| Soil sensor | GND (black) | GND |
| Soil sensor | AOUT (yellow) | GPIO 34 |

> GPIO 34 is an ADC1 pin, which keeps working while WiFi is active (ADC2 pins do not). Only submerge the sensor up to its marked line — the electronics at the top aren't waterproof.

## Software Setup

1. Install the **ESP32 board package** in Arduino IDE:
   - File → Preferences → Additional Boards Manager URLs:
     `https://espressif.github.io/arduino-esp32/package_esp32_index.json`
   - Tools → Board → Boards Manager → search "esp32" → install
2. Install libraries via Sketch → Include Library → Manage Libraries:
   - **Adafruit SSD1306**
   - **Adafruit GFX Library**
   - **DHT sensor library**
   - (Accept the prompt to also install Adafruit BusIO and Adafruit Unified Sensor)
3. If the board doesn't show up as a COM port, install the **CP2102 USB-to-UART driver** from Silicon Labs.
4. Open `plant_monitor.ino`, select **Tools → Board → ESP32 Dev Module**, pick the right COM port, and upload.

## Configuration

Edit these lines at the top of `plant_monitor.ino` before flashing:

```cpp
const char* WIFI_SSID = "YOUR_WIFI_NAME";
const char* WIFI_PASS = "YOUR_WIFI_PASSWORD";
```

> The ESP32 (classic) only supports 2.4GHz WiFi, not 5GHz.

## Calibrating the Soil Sensor

Readings vary sensor to sensor, so calibrate before trusting the percentage:

1. Flash the sketch and open the Serial Monitor at 115200 baud.
2. With the sensor in open air, note the stable `Soil raw:` value.
3. Dip just the sensor's tip in a glass of water, note that raw value too.
4. Update these two lines and re-upload:

```cpp
#define SOIL_DRY 2515  // your air reading
#define SOIL_WET 830   // your water reading
```

## Usage

Once running, the Serial Monitor and OLED will show the ESP32's IP address. Open that address in a browser on any device on the same WiFi network to see live soil moisture, temperature, and humidity.

## Troubleshooting

- **Emoji or the "°" symbol show as garbled characters on the web page:** the browser isn't being told the page is UTF-8. Make sure the HTML has `<meta charset="UTF-8">` in the `<head>`, and that `handleRoot()` sends `server.send_P(200, "text/html; charset=utf-8", PAGE);`. Both are already included in `plant_monitor.ino`.
- **"Write timeout" during upload:** usually a missing CP2102 USB driver, a charge-only USB cable, or the board needing a manual BOOT-button hold during upload.
- **Soil moisture stuck at 0% or 100% no matter what:** check that `SOIL_PIN` in the sketch matches the GPIO your sensor's signal wire is actually plugged into, and that the sensor is receiving power (3V3) and ground.

## Possible Next Steps

- Add a relay-driven water pump for automatic watering
- Push readings to a cloud dashboard (e.g. Home Assistant, Blynk, or MQTT) for history/notifications
- 3D-print or build an enclosure for the breadboard and wiring
