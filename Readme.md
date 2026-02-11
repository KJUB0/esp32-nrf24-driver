## ⚠️ Disclaimer

This software is intended for **educational and research purposes only**. 

The author is not responsible for any misuse of this software or any interference with radio communications. Users are responsible for complying with local radio frequency regulations (e.g., FCC, ETSI) regarding the monitoring and transmission on the 2.4GHz band.

## nRF24L01+ driver
In the past couple of months, I kept seeing those "bluejammers" popping up all over the internet and I decided to build one for myself. I was surprised by how effective it was, which gave me a new idea: to build a device to detect them.

This led me to implement a functioning NRF driver and that's when I decided to pivot to something even more interesting and niche: a drone detector.

When I did my research, I found really not a lot of projects that covered this type of drone detector.

This repo is a work in progress. I'm currently tuning the detection algorithm in a vibecoded Python prototype. Once I extract the key values and logic from that, I'll port them into the C code here.

#### A note on learning

This project had a steep learning curve. I used AI in the beginning to help me structure the app and find documentation, but I've reached a point where I'm handling features and bug fixes on my own.

### Project Overview

The application uses two NRF24L01+ modules simultaneously to scan the 2.4GHz ISM band for radio activity. The ESP32 drives the radios to rapidly iterate through channels, checking the Received Power Detector (RPD) bit to identify signal presence.

Data is aggregated and serialized into CSV format over UART, designed for ingestion by external visualization tools.
Current Status & Methodology

#### I am currently in the Algorithm Development phase.

* **Data Collection:** The ESP32 firmware is fully functional as a high-speed scanner, streaming raw "hit" data (signal strength > -64dBm) to a PC.
* **Python Prototyping:** I am feeding this live data into a Python environment to tune the detection algorithms. The goal is to isolate **Frequency-Hopping Spread Spectrum (FHSS)** signatures unique to common drone control protocols (e.g., signals that hop across the band faster than standard Wi-Fi).
* **Porting:** Once the detection logic is verified and tuned in Python, I will extract the key parameters and logic to implement them directly into the C firmware for a standalone, handheld device.

## Getting Started

### Prerequisites
* **ESP-IDF v5.x** installed (follow the [Espressif Guide](https://docs.espressif.com/projects/esp-idf/en/latest/esp32/get-started/)).
* **Hardware:** ESP32 DevKit V1 + 2x NRF24L01+ modules.

### Build & Flash
1.  **Clone the repo:**
    ```bash
    git clone [https://github.com/KJUB0/esp32-nrf24-driver.git](https://github.com/KJUB0/esp32-nrf24-driver.git)
    cd esp32-nrf24-driver
    ```

2.  **Build the project:**
    ```bash
    idf.py build
    ```

3.  **Flash to your ESP32** (replace `PORT` with your serial port, e.g., `COM3` or `/dev/ttyUSB0`):
    ```bash
    idf.py -p PORT flash monitor
    ```

---

## Hardware Connection
### ESP32 Pinout

| Signal | Left Radio (SPI2) | Right Radio (SPI3) |
| :--- | :--- | :--- |
| **MISO** | GPIO 19 | GPIO 12 |
| **MOSI** | GPIO 23 | GPIO 13 |
| **SCLK** | GPIO 18 | GPIO 14 |
| **CSN** | GPIO 21 | GPIO 15 |
| **CE** | GPIO 22 | GPIO 16 |

### Hardware Note
NRF24L01+ modules are very sensitive to power supply noise. I suggest to:
* Solder a **10µF to 100µF capacitor** (any voltage above 5V) directly across the `VCC` and `GND` pins of the radio module.
* Ensure you are using the ESP32's **3.3V** pin (NOT 5V!).

## Roadmap
- [x] Bare-metal SPI driver implementation
- [x] Dual-radio hardware support
- [x] High-speed channel scanning (0-83)
- [ ] Python visualization tool 
- [ ] Implement FHSS detection algorithm in C
- [ ] Port to handheld PCB or encased prototype pcb

## 📄 Copyright

© 2026 Jakub Ferencik. All Rights Reserved.
