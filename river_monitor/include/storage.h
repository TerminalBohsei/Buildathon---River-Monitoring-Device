// storage.h --> saving readings to the esp32's flash as a CSV file, and the clock
#pragma once
#include <Arduino.h>
#include "sensors.h"

// names of the current log file and the older one
#define LOG_FILE "/log.csv"
#define LOG_OLD  "/log_old.csv"

// mounts the flash storage (formats it the first time)
bool     storageBegin();

// unmounts the flash storage before sleeping
void     storageEnd();

// adds one reading to the end of the log file
void     logReading(const Reading &r);

// deletes all stored data
void     clearLog();

// number of readings logged since power-up
uint32_t readingsLogged();

// size of the log file in bytes
size_t   logSizeBytes();

// true if the clock has been set from a phone
bool timeIsSet();

// sets the clock (epochSeconds = seconds since 1970)
void setClock(long epochSeconds);
