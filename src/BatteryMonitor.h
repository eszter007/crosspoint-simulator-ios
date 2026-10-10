#pragma once
class BatteryMonitor {
public:
  BatteryMonitor(int pin, int factor = -1) {}
  void begin() {}
  int getVoltage() { return 4200; }
  int getPercentage() { return 100; }
  // Gauge chip step; false = nothing pending (the simulator has no gauge).
  static bool loadDesignCapacity() { return false; }
};
