#pragma once

#include <cstdint>
#include <cstdlib>
#include <random>

// Simulator stand-in for the ESP-IDF hardware RNG.
inline uint32_t esp_random() {
  static std::mt19937 gen{std::random_device{}()};
  return gen();
}

inline void esp_fill_random(void *buf, size_t len) {
  auto *p = static_cast<uint8_t *>(buf);
  for (size_t i = 0; i < len; ++i) p[i] = static_cast<uint8_t>(esp_random());
}
