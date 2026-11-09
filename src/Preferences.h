#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

class Preferences {
 public:
  bool begin(const char*, bool = false) { return true; }
  int64_t getLong64(const char*, int64_t defaultValue = 0) const {
    const int64_t value = floor_.load();
    return value ? value : defaultValue;
  }
  size_t putLong64(const char*, int64_t value) {
    floor_.store(value);
    return sizeof(value);
  }
  void end() {}

 private:
  inline static std::atomic<int64_t> floor_{0};
};
