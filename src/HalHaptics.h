#pragma once

#include <cstdint>

// Simulator stand-in for lib/hal/HalHaptics.h: no motor, every call is a no-op.
class HalHaptics {
 public:
  static void longPress(bool, uint8_t) {}
  static void feedback(bool, bool, uint8_t) {}
};
