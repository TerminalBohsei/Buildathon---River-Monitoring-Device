// river_monitor.ino --> main program
// DEMO_MODE 1 --> always on, constant readings, hotspot always available
// DEMO_MODE 0 --> wakes up, takes a reading, saves it and goes back to sleep
// (CAL_MODE only matters when DEMO_MODE is 0)
//
// libraries to install: OneWire, DallasTemperature, Adafruit LIS3DH, Adafruit Unified Sensor

#include <Arduino.h>
#include "config.h"
#include "sensors.h"
#include "storage.h"
#include "hotspot.h"
#include "power.h"

void setup() {
  Serial.begin(115200);
  delay(200);

  pinMode(PIN_WAKE, INPUT_PULLUP);
  sensorsBegin();
  if (!storageBegin()) Serial.println("Flash storage failed to mount");

#if DEMO_MODE
  Serial.println("DEMO MODE - always on, constant readings");
  sensorsSetAlwaysOn(true);   // keep the sensors powered between readings
  hotspotStart();

#elif CAL_MODE
  Serial.println("CALIBRATION MODE - hotspot stays on, values print every 2 s");
  hotspotStart();

#else
  if (esp_sleep_get_wakeup_cause() != ESP_SLEEP_WAKEUP_TIMER) {
    hotspotRunUntilIdle();
  }
  Reading r = takeReading();
  printReading(r);
  logReading(r);
  goToSleep();
#endif
}

void loop() {
#if DEMO_MODE
  static uint32_t lastRead = 0;
  static uint32_t lastLog = 0;

  hotspotHandle();   // answer web page requests

  if (millis() - lastRead >= DEMO_READ_MS) {
    lastRead = millis();
    Reading r = takeReading();
    printReading(r);

    // save to flash less often so it doesn't fill up
    if (millis() - lastLog >= DEMO_LOG_MS) {
      lastLog = millis();
      logReading(r);
    }
  }

#elif CAL_MODE
  static uint32_t lastPrint = 0;
  hotspotHandle();
  if (millis() - lastPrint > 2000) {
    lastPrint = millis();
    printReading(takeReading());
  }
#endif
}