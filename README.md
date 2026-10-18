# Buidathon 2026 - River-Monitoring-Device
***Team Kohsei's Kohai***

## Overview
A low-cost, solar-powered monitoring device designed to be placed in rivers and streams. It measures water temperature, turbidity, dissolved solids (TDS) and water level, then logs the readings and uploads them wirelessly so water quality and river conditions can be tracked over time without anyone needing to be on site.

**Objective:**
Provide continuous, affordable, low-maintenance river monitoring that can detect changes in water quality (e.g. sediment from runoff or erosion) and rising water levels (e.g. flood risk) early.

## Competition Objective / Rules:
**Objective:**
Design a mechatronic solution that helps address environmental damage with climate change in mind. Teams must do this by choosing **at least one** of three subcatergories: **Monitoring**, **Repurposing** and **Restoration** and meet at least **one** of the listed criteria for the chosen subcategory.

## Hardware
- 1x ESP32 (Given in starter kit)
- 1x Waterproof DS18B20 digital temperature sensor (A$11.65)
- 1x DFRobot SEN0244 Gravity analog TDS sensor (A$21.25)
- 1x DFRobot SEN0189 Gravity analog turbidity sensor (A$16.40)
- 1x PiicoDev LIS3DH 3-axis accelerometer (A$5.25) - measures tilt of a float arm to infer water level
- 1x 5 V boost converter, CE05174 (A$0.63)
- 1x PG7 cable gland 5-pack (A$3.50)
- 1x Rechargeable battery pack (NiMH)
- 1x Small solar panel
- Misc: 4.7k resistor (DS18B20 pull-up), 10k + 20k resistors (turbidity voltage divider), enclosure, float arm

Sensor cost (excluding power parts): A$58.68

## Software
Language: C++

Platform: Arduino(ESP32)

IDE: Arduino IDE

Libraries: OneWire, DallasTemperature, Adafruit LIS3DH, Adafruit Unified Sensor

## General Function / How it works:
1. ESP32 wakes from deep sleep on a timer (default every 15 minutes)
2. Powers the sensors, waits for them to settle, and takes filtered readings
3. Temperature compensates the TDS reading
4. Converts float-arm tilt into a water level in cm
5. Stores the reading in RTC memory, connects to WiFi, and uploads all buffered readings as JSON
6. Clears the buffer on a successful upload, then goes back to sleep

## Repository Structure
src/ - main sketch (river_monitor.ino)

include/ - pin definitions and configuration headers

hardware/ - wiring diagrams, enclosure and float arm designs

docs/ - calibration notes, results, competition write-up

tests/ - individual sensor test sketches
