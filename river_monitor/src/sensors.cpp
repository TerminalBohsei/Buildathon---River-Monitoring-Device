#include <Arduino.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <Adafruit_LIS3DH.h>
#include <Adafruit_Sensor.h>
#include <driver/gpio.h>
#include "config.h"
#include "sensors.h"

// objects for the temperature sensor and the accelerometer
static OneWire oneWire(PIN_ONEWIRE);
static DallasTemperature ds18(&oneWire);
static Adafruit_LIS3DH lis = Adafruit_LIS3DH();

// true if the accelerometer was found
static bool lisOk = false;

// sensorPower --> turns the sensor power switch on or off
static void sensorPower(bool on) {
  pinMode(PIN_SENSOR_PWR, OUTPUT);
  // LOW = on, HIGH = off (flip these if your switch works the other way)
  digitalWrite(PIN_SENSOR_PWR, on ? LOW : HIGH);
}

// sensorsBegin --> sets up the pins and i2c
void sensorsBegin() {
  // release the power pin if it was held during sleep
  gpio_hold_dis((gpio_num_t)PIN_SENSOR_PWR);

  // adc settings for the analog sensors
  analogReadResolution(12);
  analogSetPinAttenuation(PIN_TDS, ADC_11db);
  analogSetPinAttenuation(PIN_TURB, ADC_11db);

  // start i2c and keep the sensors off until needed
  Wire.begin(PIN_I2C_SDA, PIN_I2C_SCL);
  sensorPower(false);
}

// sensorsPrepareForSleep --> turns sensors off and holds the pin off during sleep
void sensorsPrepareForSleep() {
  sensorPower(false);
  gpio_hold_en((gpio_num_t)PIN_SENSOR_PWR);
  gpio_deep_sleep_hold_en();
}

// readMilliVoltsMedian --> reads a pin n times and returns the middle value in millivolts
static float readMilliVoltsMedian(int pin, int n = 31) {
  uint32_t v[64];
  if (n > 64) n = 64;

  // take n readings
  for (int i = 0; i < n; i++) {
    v[i] = analogReadMilliVolts(pin);
    delay(10);
  }

  // sort the readings from smallest to biggest
  for (int i = 1; i < n; i++) {
    uint32_t k = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > k) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = k;
  }

  // the middle one ignores any random spikes
  return (float)v[n / 2];
}

// readTempC --> reads the water temperature in deg C (NAN if the sensor fails)
static float readTempC() {
  ds18.begin();
  ds18.setResolution(12);

  // try up to 3 times
  for (int attempt = 0; attempt < 3; attempt++) {
    ds18.requestTemperatures();
    float t = ds18.getTempCByIndex(0);

    // -127 means disconnected and 85 means a bad startup read, anything else is good
    if (t != DEVICE_DISCONNECTED_C && t != 85.0f) return t;
    delay(200);
  }
  return NAN;
}

// readTDS --> reads the TDS sensor and returns ppm (uses temperature to correct it)
static float readTDS(float tempC) {
  float v = readMilliVoltsMedian(PIN_TDS) / 1000.0f;

  // assume 25C if the temperature reading failed
  if (isnan(tempC)) tempC = 25.0f;

  // correct the voltage for the water temperature
  float comp = 1.0f + 0.02f * (tempC - 25.0f);
  float vc = v / comp;

  // convert voltage to ppm
  float tds = (133.42f * vc * vc * vc - 255.86f * vc * vc + 857.39f * vc) * 0.5f;
  return tds < 0 ? 0 : tds;
}

// readTurbidity --> reads the turbidity sensor and returns NTU (also gives back the voltage)
static float readTurbidity(float &sensorVolts) {
  float pinMv = readMilliVoltsMedian(PIN_TURB);

  // undo the voltage divider to get the real sensor voltage
  sensorVolts = (pinMv / 1000.0f) * TURB_DIVIDER;
  float v = sensorVolts;

  // convert voltage to NTU (below 2.5V is the max turbidity)
  float ntu;
  if (v < 2.5f) ntu = 3000.0f;
  else ntu = -1120.4f * v * v + 5742.3f * v - 4352.9f;
  return ntu < 0 ? 0 : ntu;
}

// readTiltDeg --> reads the float arm tilt angle in degrees (NAN if no accelerometer)
static float readTiltDeg() {
  if (!lisOk) return NAN;

  // add up 25 readings so we can average them
  float sx = 0, sy = 0, sz = 0;
  const int n = 25;
  for (int i = 0; i < n; i++) {
    sensors_event_t e;
    lis.getEvent(&e);
    sx += e.acceleration.x;
    sy += e.acceleration.y;
    sz += e.acceleration.z;
    delay(10);
  }

  // average the readings
  sx /= n;
  sy /= n;
  sz /= n;

  // work out the tilt angle from gravity
  return atan2f(sx, sqrtf(sy * sy + sz * sz)) * 180.0f / PI;
}

// tiltToLevelCm --> converts the tilt angle into a water level in cm
static float tiltToLevelCm(float deg) {
  if (isnan(deg)) return NAN;

  // how far between the dry and full angle (0 to 1)
  float f = (deg - ANGLE_DRY_DEG) / (ANGLE_FULL_DEG - ANGLE_DRY_DEG);
  f = constrain(f, 0.0f, 1.0f);
  return f * LEVEL_RANGE_CM;
}

// takeReading --> turns on the sensors, reads all of them and turns them off again
Reading takeReading() {
  Reading r;
  sensorPower(true);

  // wait for the sensors to settle
  delay(1500);

  // start the accelerometer again since it may have lost power
  lisOk = lis.begin(LIS3DH_ADDR);
  if (lisOk) lis.setRange(LIS3DH_RANGE_2_G);
  else Serial.println("LIS3DH not found - check wiring/address");

  // read each sensor
  r.tempC    = readTempC();
  r.tds_ppm  = readTDS(r.tempC);
  r.turb_ntu = readTurbidity(r.turb_v);
  r.tilt_deg = readTiltDeg();
  r.level_cm = tiltToLevelCm(r.tilt_deg);

  sensorPower(false);
  return r;
}

// printReading --> prints one reading to the serial monitor
void printReading(const Reading &r) {
  Serial.printf("temp=%.2fC  TDS=%.0fppm  turb=%.0fNTU (%.3fV)  tilt=%.1fdeg  level=%.1fcm\n",
                r.tempC, r.tds_ppm, r.turb_ntu, r.turb_v, r.tilt_deg, r.level_cm);
}
