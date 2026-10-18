#include <Arduino.h>
#include <LittleFS.h>
#include <sys/time.h>
#include <time.h>
#include "config.h"
#include "storage.h"

// counts readings logged, kept in rtc memory so it survives deep sleep (resets if power is lost)
RTC_DATA_ATTR static uint32_t seqNo = 0;

// storageBegin --> mounts the flash (true = format it if mounting fails)
bool storageBegin() { return LittleFS.begin(true); }

// storageEnd --> unmounts the flash
void storageEnd()   { LittleFS.end(); }

// readingsLogged --> returns how many readings were logged since power-up
uint32_t readingsLogged() { return seqNo; }

// logSizeBytes --> returns the size of the log file (0 if it doesn't exist)
size_t logSizeBytes() {
  if (!LittleFS.exists(LOG_FILE)) return 0;
  File f = LittleFS.open(LOG_FILE, FILE_READ);
  size_t sz = f.size();
  f.close();
  return sz;
}

// clearLog --> deletes both log files and resets the reading count
void clearLog() {
  LittleFS.remove(LOG_FILE);
  LittleFS.remove(LOG_OLD);
  seqNo = 0;
}

// timeIsSet --> checks if the clock has been set (it starts at 1970 until it is)
bool timeIsSet() { return time(nullptr) > 1700000000; }

// setClock --> sets the clock from a unix timestamp
void setClock(long epochSeconds) {
  if (epochSeconds > 1700000000) {
    struct timeval tv = { (time_t)epochSeconds, 0 };
    settimeofday(&tv, nullptr);
  }
}

// csvNum --> turns a number into text for the csv (blank if the reading failed)
static String csvNum(float v, int dp) { return isnan(v) ? String("") : String(v, dp); }

// logReading --> adds one reading as a new row in the csv file
void logReading(const Reading &r) {
  // if the log is too big, move it to the old file and start a new one
  if (logSizeBytes() > MAX_LOG_BYTES) {
    LittleFS.remove(LOG_OLD);
    LittleFS.rename(LOG_FILE, LOG_OLD);
  }

  // open the log file (adds to the end)
  bool newFile = !LittleFS.exists(LOG_FILE);
  File f = LittleFS.open(LOG_FILE, FILE_APPEND);
  if (!f) { Serial.println("Could not open log file"); return; }

  // write the column names if this is a new file
  if (newFile) {
    f.println("seq,epoch_utc,datetime_utc,temp_c,tds_ppm,turbidity_ntu,turbidity_v,tilt_deg,level_cm");
  }

  // get the current time as text (left blank if the clock isn't set yet)
  time_t now = time(nullptr);
  char ts[24] = "";
  if (timeIsSet()) {
    struct tm t;
    gmtime_r(&now, &t);
    strftime(ts, sizeof ts, "%Y-%m-%d %H:%M:%S", &t);
  } else {
    now = 0;
  }

  // write the row and close the file
  seqNo++;
  f.printf("%lu,%lu,%s,%s,%s,%s,%s,%s,%s\n",
           (unsigned long)seqNo, (unsigned long)now, ts,
           csvNum(r.tempC, 2).c_str(), csvNum(r.tds_ppm, 0).c_str(),
           csvNum(r.turb_ntu, 0).c_str(), csvNum(r.turb_v, 3).c_str(),
           csvNum(r.tilt_deg, 1).c_str(), csvNum(r.level_cm, 1).c_str());
  f.close();
}
