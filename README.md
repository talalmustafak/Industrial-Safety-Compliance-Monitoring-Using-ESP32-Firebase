# Industrial Safety Compliance Monitoring

## Overview
Industrial environments pose various safety risks, including toxic gas emissions, extreme temperatures, and unauthorized access. This project implements an **Industrial Safety Compliance Monitoring** system using an **ESP32 microcontroller** integrated with multiple sensors to ensure worker safety and compliance with safety standards.

## Features
- **Real-time monitoring** of environmental parameters
- **WiFi connectivity** for remote data logging
- **OLED display** for local data visualization
- **Firebase integration** for cloud-based monitoring
- **Automated alerts** for hazardous conditions

## Components Used
### Hardware
- **ESP32 Microcontroller**
- **MQ2 & MQ135 Gas Sensors** (detects toxic gases)
- **DHT22 Temperature & Humidity Sensor**
- **HCSR04 Ultrasonic Sensor** (distance measurement)
- **HCSR501 Motion Sensor**
- **OLED Display (128x64)**
- **WiFi connectivity for Firebase integration**

### Software
- **Arduino IDE** (for programming the ESP32)
- **Firebase Realtime Database** (for cloud storage)
- **C/C++** (for firmware development)
- **Python (optional)** for data analysis

## System Working
1. Sensors continuously collect data.
2. ESP32 processes the data and compares it with predefined safety thresholds.
3. If a hazardous condition is detected:
   - A **local alert** is displayed on the OLED.
   - Data is **transmitted to Firebase** for remote monitoring.
4. Data is accessible through a cloud-based dashboard for safety monitoring.

## Hardware Connections
| Sensor | ESP32 Pin |
|--------|----------|
| MQ2 Gas Sensor | GPIO34 |
| MQ135 Gas Sensor | GPIO32 |
| DHT22 Temperature Sensor | GPIO15 |
| HCSR04 Ultrasonic Sensor | GPIO5 (Trigger), GPIO18 (Echo) |
| HCSR501 Motion Sensor | GPIO27 |
| OLED Display | I2C (SDA, SCL) |

## Installation & Setup
1. **Clone the Repository**
2. **Install Required Libraries**
   - ESP32 Board in Arduino IDE
   - Adafruit SSD1306 (OLED Display)
   - Firebase ESP32 Library
3. **Upload Code to ESP32**
   - Connect ESP32 to your PC.
   - Select the correct board and COM port in Arduino IDE.
   - Upload the code.
4. **Set up Firebase Database**
   - Create a Firebase project.
   - Configure database rules and get API credentials.
   - Update the code with your Firebase credentials.
