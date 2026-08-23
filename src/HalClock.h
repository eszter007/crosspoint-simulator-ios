#pragma once

#include <Arduino.h>

#include <cstddef>
#include <cstdint>
#include <ctime>

class HalClock;
extern HalClock halClock;

class HalClock {
  bool _available = false;

 public:
  void begin();
  bool isAvailable() const { return _available; }
  bool getTime(uint8_t& hour, uint8_t& minute) const;
  bool getDateTime(uint16_t& year, uint8_t& month, uint8_t& day, uint8_t& hour, uint8_t& minute) const;
  bool formatTime(char* buf, size_t bufSize,
                  uint8_t utcOffsetQuarterHoursBiased = 48,
                  bool use12Hour = false) const;
  bool formatDate(char* buf, size_t bufSize,
                  uint8_t utcOffsetQuarterHoursBiased = 48) const;
  bool syncFromNTP();

  // True when the system clock has ever been set. On a host it always has been
  // -- there is no unset-RTC state to recover from.
  static bool systemTimeValid();
  // On device these stash the epoch to SD and seed the clock from it at boot,
  // because the RTC loses time across a power cut. The host clock is already
  // correct, so both are no-ops here.
  void restoreSystemTime() const;
  void persistSystemTime() const;

  // Epoch shifted into the configured local zone (offset in quarter hours,
  // biased by 48, so 48 = UTC). For DATE decisions such as reading-stats day
  // boundaries, so days flip at local midnight rather than UTC midnight.
  static time_t localEpoch(uint8_t utcOffsetQuarterHoursBiased);
};
