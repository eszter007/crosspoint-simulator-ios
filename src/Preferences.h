#pragma once

#include <cstdint>
#include <map>
#include <string>

// Simulator stand-in for the ESP32 Preferences (NVS) API: an in-memory store
// that lives for the process. Enough for TrustedTime's single int64 slot.
class Preferences {
 public:
  bool begin(const char *name, bool readOnly = false) {
    (void)readOnly;
    ns_ = name ? name : "";
    return true;
  }
  void end() {}
  int64_t getLong64(const char *key, int64_t fallback = 0) {
    const auto it = store().find(ns_ + "/" + key);
    return it == store().end() ? fallback : it->second;
  }
  size_t putLong64(const char *key, int64_t value) {
    store()[ns_ + "/" + key] = value;
    return sizeof(value);
  }

 private:
  static std::map<std::string, int64_t> &store() {
    static std::map<std::string, int64_t> s;
    return s;
  }
  std::string ns_;
};
