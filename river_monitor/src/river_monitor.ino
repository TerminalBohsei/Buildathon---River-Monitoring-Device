// river_monitor.ino --> main program, wakes up, takes a reading, saves it and goes back to sleep
// to download the data: touch a wire from GPIO 33 to GND, connect to the "RiverMonitor" wifi and open http://192.168.4.1
//
// files in this folder:
// config.h --> settings, pins and calibration values
// sensors --> reads the four sensors
// storage --> saves readings to flash and handles the clock
// hotspot --> wifi hotspot and download web page
// power --> deep sleep
//
// libraries to install: OneWire, DallasTemperature, Adafruit LIS3DH, Adafruit Unified Sensor

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "storage.h"
#include "hotspot.h"
#include "power.h"

// setup --> runs once every time the esp32 wakes up
void setup() {
  Serial.begin(115200);
  delay(200);

  // set up the wake pin, sensors and flash storage
  pinMode(PIN_WAKE, INPUT_PULLUP);
  sensorsBegin();
  if (!storageBegin()) Serial.println("Flash storage failed to mount");

#if CAL_MODE
  // calibration mode: leave the hotspot on, readings print in loop()
  Serial.println("CALIBRATION MODE - hotspot stays on, values print every 2 s");
  hotspotStart();
#else
  // if woken by the wake wire or first power-up, open the hotspot so data can be downloaded
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
    hotspotRunUntilIdle();
  }

  // take a reading, print it and save it
  Reading r = takeReading();
  printReading(r);
  logReading(r);

  // go back to sleep until the next reading
  goToSleep();
#endif
}

// loop --> only used in calibration mode
void loop() {
#if CAL_MODE
  static uint32_t lastPrint = 0;

  // answer any web page requests
  hotspotHandle();

  // print a new reading every 2 seconds
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    printReading(takeReading());
  }
#endif
}
