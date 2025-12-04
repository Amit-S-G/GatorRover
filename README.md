# GatorRover

A comprehensive remote-controlled land rover system designed for remote observation of disaster zones and high-risk areas where direct human intervention may be too dangerous. The system features wireless glove-based control, environmental sound monitoring, and live video streaming capabilities.

Developed by: Sunny Gupta, Corey Phillips, Chenfeng Su, and Michael Yao

---

## Table of Contents
- [System Overview](#system-overview)
- [Features](#features)
- [System Architecture](#system-architecture)
- [Hardware Requirements](#hardware-requirements)
- [Software Requirements](#software-requirements)
- [Setup Instructions](#setup-instructions)
  - [1. Glove Controller Setup](#1-glove-controller-setup)
  - [2. Sensing Drone Setup](#2-sensing-drone-setup)
  - [3. Video Streaming Server Setup](#3-video-streaming-server-setup)
- [Pin Configurations](#pin-configurations)
- [Usage](#usage)
- [Communication Protocol](#communication-protocol)
- [Environment Configuration](#environment-configuration)
- [Troubleshooting](#troubleshooting)
- [Future Improvements](#future-improvements)

---

## System Overview

GatorRover consists of three main components working together:

1. **Glove Controller** - Wireless joystick-based controller worn as a glove that sends motor commands and receives environmental feedback
2. **Sensing Drone** - The rover platform with motor control, environmental sound detection via I2S microphone, and camera capabilities
3. **Video Streaming Server** - Node.js server that receives camera frames from the drone and streams live video to authenticated web clients

---

## Features

### Glove Controller
- Dual joystick control (left and right motors independently)
- NRF24L01 wireless transmission (2.4GHz)
- Real-time feedback via LED indicator
- Loud sound detection alerts from drone
- Low-latency command transmission

### Sensing Drone
- Differential drive motor control (tank-style steering)
- I2S microphone for environmental sound monitoring
- Automatic loud sound detection with configurable threshold
- Bi-directional wireless communication
- H-bridge motor drivers for dual motor control
- Live camera streaming (via separate ESP32-CAM)

### Video Streaming Server
- MJPEG video streaming over HTTPS
- Basic authentication for secure access
- API key authentication for ESP32 camera uploads
- Responsive web interface with real-time status indicators
- Configurable frame rate
- Cross-platform support (local development and VPS deployment)

---

## System Architecture

```
┌─────────────────┐         NRF24 Radio          ┌──────────────────┐
│ Glove Controller│    ◄───── 2.4GHz ─────►      │  Sensing Drone   │
│   (ESP32 #1)    │                              │    (ESP32 #2)    │
│                 │   Command Codes (0-8)        │                  │
│  - 2x Joysticks │  ──────────────────►         │  - 2x Motors     │
│  - LED Alert    │                              │  - I2S Mic       │
│                 │   Sound Status (0/1)         │  - Motor Drivers │
│                 │  ◄──────────────────          │                  │
└─────────────────┘                              └──────────────────┘
                                                          │
                                                          │ HTTPS
                                                  Camera Frames
                                                          │
                                                          ▼
                                                 ┌─────────────────┐
                                                 │  Video Server   │
                                                 │   (Node.js)     │
                                                 │                 │
                                                 │  - MJPEG Stream │
                                                 │  - Web UI       │
                                                 │  - Auth Layer   │
                                                 └─────────────────┘
                                                          │
                                                          │ HTTPS
                                                          ▼
                                                 ┌─────────────────┐
                                                 │   Web Browser   │
                                                 │  (User Client)  │
                                                 └─────────────────┘
```

---

## Hardware Requirements

### Glove Controller
- **Microcontroller:** ESP32 development board
- **Radio Module:** NRF24L01+ transceiver
- **Input:** 2x analog joysticks (potentiometer-based)
- **Output:** 1x LED indicator
- **Power:** USB or battery (3.3V/5V compatible)

### Sensing Drone
- **Microcontroller:** ESP32 development board
- **Radio Module:** NRF24L01+ transceiver
- **Microphone:** INMP441 I2S digital microphone
- **Motor Drivers:** 2x H-bridge motor drivers (e.g., L298N, DRV8833)
- **Motors:** 2x DC motors with wheels
- **Camera:** ESP32-CAM module (separate)
- **Chassis:** Robot chassis with mounting hardware
- **Power:** Battery pack (suitable for motors and ESP32)

### Video Streaming Server
- **Server:** Linux/Windows/macOS machine or VPS
- **Network:** Stable internet connection
- **Optional:** SSL certificates for HTTPS

---

## Software Requirements

### Arduino Components
- Arduino IDE 1.8.x or 2.x
- ESP32 board support package
- Libraries:
  - `RadioHead` (for NRF24L01 communication)
  - `SPI` (for NRF24 SPI interface)
  - `driver/i2s.h` (ESP32 I2S driver, built-in)

### Server Component
- Node.js v18.20.4 or higher
- npm v9.2.0 or higher
- Dependencies (installed via npm):
  - `express`
  - `express-basic-auth`
  - `dotenv`

---

## Setup Instructions

### 1. Glove Controller Setup

#### Wiring
Connect components according to the pin configuration:
- **NRF24L01:**
  - CE → GPIO 4
  - CSN → GPIO 5
  - MOSI, MISO, SCK → SPI pins
- **Joysticks:**
  - Left joystick → GPIO 35 (analog)
  - Right joystick → GPIO 34 (analog)
- **LED:**
  - Positive → GPIO 2
  - Negative → GND (through resistor)

#### Software Installation
1. Install Arduino IDE and ESP32 board support
2. Install RadioHead library via Library Manager
3. Open `glove/glove.ino`
4. Adjust joystick thresholds if needed:
   ```cpp
   int forwardThreshold = 30;      // Tune for your joysticks
   int backwardThreshold = 1800;   // Tune for your joysticks
   ```
5. Upload to ESP32

### 2. Sensing Drone Setup

#### Wiring
Connect all components according to pin configurations:

**NRF24L01:**
- CE → GPIO 4
- CSN → GPIO 5

**INMP441 I2S Microphone:**
- WS (LRCLK) → GPIO 25
- SD (DOUT) → GPIO 33
- SCK (BCLK) → GPIO 32

**Left Motor H-Bridge:**
- IN1 → GPIO 0
- IN2 → GPIO 15

**Right Motor H-Bridge:**
- IN1 → GPIO 13
- IN2 → GPIO 2

#### Software Installation
1. Install required libraries (RadioHead)
2. Open `sensingDrone/sensingDrone.ino`
3. Adjust sound detection threshold if needed:
   ```cpp
   #define THRESHOLD 600    // Adjust based on environment
   #define WINDOW_MS 500    // Averaging window
   ```
4. Upload to ESP32
5. Connect H-bridges to motors and power supply

### 3. Video Streaming Server Setup

#### Installation
1. Clone the repository:
   ```bash
   git clone https://github.com/Amit-S-G/GatorRover.git
   cd GatorRover/server
   ```

2. Install dependencies:
   ```bash
   npm install
   ```

3. Create `.env` file in `server/` directory:
   ```env
   browser_username=your_username
   browser_password=your_password
   certs_path_vm=/path/to/certs  # For VPS deployment
   certs_path_local=/path/to/certs  # For local development
   esp32_api_key=your_secret_api_key
   NODE_ENV=LOCAL  # or VM for VPS
   ```

4. Run the server:
   
   **Local Development:**
   ```bash
   npm run start-local
   ```
   
   **VPS Deployment:**
   ```bash
   npm run start-vm
   ```

5. Access the web interface:
   - Navigate to `http://localhost:8443/view` (local)
   - Or your VPS address (production)
   - Enter credentials from `.env` file

---

## Pin Configurations

### Glove Controller (ESP32 #1)
| Component | Pin | Function |
|-----------|-----|----------|
| NRF24 CE | GPIO 4 | Chip Enable |
| NRF24 CSN | GPIO 5 | Chip Select |
| Left Joystick | GPIO 35 | Analog Input |
| Right Joystick | GPIO 34 | Analog Input |
| LED | GPIO 2 | Alert Output |

### Sensing Drone (ESP32 #2)
| Component | Pin | Function |
|-----------|-----|----------|
| NRF24 CE | GPIO 4 | Chip Enable |
| NRF24 CSN | GPIO 5 | Chip Select |
| I2S WS (LRCLK) | GPIO 25 | Word Select |
| I2S SD (DOUT) | GPIO 33 | Serial Data |
| I2S SCK (BCLK) | GPIO 32 | Serial Clock |
| Left Motor IN1 | GPIO 0 | H-Bridge Input |
| Left Motor IN2 | GPIO 15 | H-Bridge Input |
| Right Motor IN1 | GPIO 13 | H-Bridge Input |
| Right Motor IN2 | GPIO 2 | H-Bridge Input |

---

## Usage

### Operating the Rover

1. **Power on** both the glove controller and sensing drone
2. **Start the video server** and open the web interface
3. **Control the rover** using the joysticks:
   - Left joystick: Controls left motor
   - Right joystick: Controls right motor
   - Forward: Push joystick forward
   - Backward: Pull joystick backward
   - Stop: Center position

4. **Monitor feedback**:
   - LED on glove lights up when loud sound detected
   - Web interface shows live camera feed
   - Serial monitors show real-time debug info

### Web Interface Features
- Real-time video streaming
- Connection status indicator
- Responsive design
- Authentication-protected access

---

## Communication Protocol

### Motor Command Encoding
Commands are encoded as single bytes (0-8):
```
code = (leftMotor + 1) * 3 + (rightMotor + 1)

Where motor values are:
  -1 = Reverse
   0 = Stop
   1 = Forward

Example codes:
  0 = Both reverse
  4 = Both stop
  8 = Both forward
  3 = Left stop, right forward
```

### Sound Detection Response
The drone sends back a single byte:
- `0` = Normal sound levels
- `1` = Loud sound detected (above threshold)

### Radio Configuration
- **Channel:** 67 (2.467 GHz)
- **Data Rate:** 2 Mbps
- **Transmit Power:** 0 dBm
- **Communication:** Bi-directional

---

## Environment Configuration

The `.env` file in `server/` must contain:

```env
# Web Interface Credentials
browser_username=admin
browser_password=secure_password_here

# SSL Certificate Paths
certs_path_vm=/etc/letsencrypt/live/yourdomain.com
certs_path_local=./certs

# ESP32 Camera Authentication
esp32_api_key=your_secret_key_here

# Deployment Environment
NODE_ENV=LOCAL  # or VM
```

**Security Notes:**
- Never commit `.env` to version control
- Use strong passwords for browser authentication
- Keep API keys secret and rotate regularly
- Use HTTPS in production

---

## Troubleshooting

### Glove Controller Issues
- **No response from drone:** Check NRF24 wiring and channel setting
- **Joysticks not working:** Verify analog pin readings via Serial Monitor, adjust thresholds
- **LED not lighting:** Check LED polarity and GPIO 2 connection

### Sensing Drone Issues
- **Motors not responding:** Verify H-bridge connections and power supply
- **Microphone not detecting sound:** Check I2S wiring and threshold settings
- **NRF24 communication failed:** Ensure both modules on same channel (67)

### Server Issues
- **Cannot access web interface:** Check firewall settings and port 8443
- **No video stream:** Verify ESP32-CAM is uploading frames with correct API key
- **Authentication fails:** Check `.env` credentials match login attempt

### General Debug Steps
1. Enable Serial Monitor (115200 baud for drone, 9600 for glove)
2. Check power supply voltages
3. Verify all ground connections are shared
4. Test NRF24 modules individually
5. Check library versions match

---

## Future Improvements

- Integrate camera code with motor control on single ESP32
- Add battery voltage monitoring
- Implement obstacle detection sensors
- Add GPS tracking capability
- Implement PWM speed control for motors
- Add emergency stop button
- Support multiple video quality settings
- Add telemetry dashboard
- Implement autonomous navigation mode
- Add night vision camera option

---

## License

This project is developed as part of the University of Florida GatorRover Project.

## Contributors

- Sunny Gupta
- Corey Phillips
- Chenfeng Su
- Michael Yao

---

## Contact

For questions or issues, please open an issue on the GitHub repository.