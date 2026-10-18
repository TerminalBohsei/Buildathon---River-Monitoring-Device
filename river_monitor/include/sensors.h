// sensors.h --> reading the temperature, TDS, turbidity and water level sensors
#pragma once

// Reading --> holds one full set of sensor values
struct Reading {
  float tempC;      // water temperature (deg C)
  float tds_ppm;    // total dissolved solids
  float turb_ntu;   // turbidity (approximate NTU)
  float turb_v;     // turbidity sensor voltage (used for calibration)
  float level_cm;   // water level
  float tilt_deg;   // float arm tilt (used for calibration)
};

// sets up the pins and i2c, call once at startup
void sensorsBegin();

// powers the sensors, reads all of them, then powers them off
Reading takeReading();

// prints a reading to the serial monitor
void printReading(const Reading &r);

// keeps the sensor power switch off while the esp32 is asleep
void sensorsPrepareForSleep();
