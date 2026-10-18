// config.h --> all the settings, pins and calibration values (edit things here)
#pragma once

// 1 = calibration mode (hotspot stays on and readings print every 2 seconds)
// 0 = deployment mode (sleeps between readings and logs to flash)
#define CAL_MODE            1

// time between readings in seconds (15 minutes)
#define SAMPLE_INTERVAL_S   (15 * 60)

// name and password of the esp32's wifi hotspot (password needs 8+ characters)
#define AP_SSID             "RiverMonitor"
#define AP_PASS             "river1234"

// hotspot turns off after 3 minutes with no one connected
#define AP_IDLE_MS          (3UL * 60UL * 1000UL)

// hotspot turns off after 30 minutes no matter what
#define AP_MAX_MS           (30UL * 60UL * 1000UL)

// log file starts a new file after it gets this big (bytes)
#define MAX_LOG_BYTES       900000

// pin for the DS18B20 temperature sensor (needs 4.7k pull-up to 3.3V)
#define PIN_ONEWIRE    4

// pin for the TDS sensor output
#define PIN_TDS        34

// pin for the turbidity sensor output (goes through the voltage divider)
#define PIN_TURB       35

// i2c pins for the LIS3DH accelerometer
#define PIN_I2C_SDA    21
#define PIN_I2C_SCL    22

// touch a wire from this pin to GND to wake the device and start the hotspot
#define PIN_WAKE       33

// pin that switches power to the sensors on and off (optional)
#define PIN_SENSOR_PWR 25

// i2c address of the LIS3DH (0x19 is the PiicoDev default)
#define LIS3DH_ADDR    0x19

// turbidity voltage divider resistors (sensor OUT --> R1 --> pin --> R2 --> GND)
#define TURB_R1  10000.0f
#define TURB_R2  20000.0f

// multiply the pin voltage by this to get the real sensor voltage
#define TURB_DIVIDER ((TURB_R1 + TURB_R2) / TURB_R2)

// float arm tilt angle when the water is at the lowest mark
#define ANGLE_DRY_DEG    10.0f

// float arm tilt angle when the water is at the highest mark
#define ANGLE_FULL_DEG   70.0f

// water height between the lowest and highest mark (cm)
#define LEVEL_RANGE_CM   100.0f
