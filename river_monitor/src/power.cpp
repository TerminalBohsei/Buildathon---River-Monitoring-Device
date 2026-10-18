#include <Arduino.h>
#include <driver/rtc_io.h>
#include "config.h"
#include "sensors.h"
#include "storage.h"
#include "power.h"

// goToSleep --> shuts everything down and puts the esp32 into deep sleep
void goToSleep() {
  Serial.println("Sleeping...");
  Serial.flush();

  // unmount the flash and keep the sensors off
  storageEnd();
  sensorsPrepareForSleep();

  // wake up after the sample interval
  esp_sleep_enable_timer_wakeup((uint64_t)SAMPLE_INTERVAL_S * 1000000ULL);

  // also wake up when the wake pin is pulled to GND
  // (skipped if the wire is already connected, otherwise it would wake up straight away)
  if (digitalRead(PIN_WAKE) == HIGH) {
    esp_sleep_enable_ext0_wakeup((gpio_num_t)PIN_WAKE, 0);
    rtc_gpio_pullup_en((gpio_num_t)PIN_WAKE);
    rtc_gpio_pulldown_dis((gpio_num_t)PIN_WAKE);
  }

  // go to sleep
  esp_deep_sleep_start();
}
