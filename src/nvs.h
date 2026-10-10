#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <string>
#include <vector>

#include "esp_err.h"

// Simulator stand-in for the ESP-IDF NVS blob API: an in-memory store for the
// process, keyed by namespace/key. Enough for DeviceSecret's single blob.
using nvs_handle_t = uint32_t;
enum nvs_open_mode_t { NVS_READONLY = 0, NVS_READWRITE = 1 };
#ifndef ESP_ERR_NVS_NOT_FOUND
#define ESP_ERR_NVS_NOT_FOUND 0x1102
#endif

namespace nvs_sim {
inline std::map<std::string, std::vector<uint8_t>> &store() {
  static std::map<std::string, std::vector<uint8_t>> s;
  return s;
}
inline std::map<nvs_handle_t, std::string> &namespaces() {
  static std::map<nvs_handle_t, std::string> n;
  return n;
}
}  // namespace nvs_sim

inline esp_err_t nvs_open(const char *name, int /*mode*/, nvs_handle_t *out) {
  static nvs_handle_t next = 1;
  *out = next++;
  nvs_sim::namespaces()[*out] = name ? name : "";
  return ESP_OK;
}
inline esp_err_t nvs_get_blob(nvs_handle_t h, const char *key, void *out, size_t *len) {
  const auto it = nvs_sim::store().find(nvs_sim::namespaces()[h] + "/" + key);
  if (it == nvs_sim::store().end()) return ESP_ERR_NVS_NOT_FOUND;
  if (out) memcpy(out, it->second.data(), std::min(*len, it->second.size()));
  *len = it->second.size();
  return ESP_OK;
}
inline esp_err_t nvs_set_blob(nvs_handle_t h, const char *key, const void *data, size_t len) {
  const auto *p = static_cast<const uint8_t *>(data);
  nvs_sim::store()[nvs_sim::namespaces()[h] + "/" + key] = std::vector<uint8_t>(p, p + len);
  return ESP_OK;
}
inline esp_err_t nvs_commit(nvs_handle_t) { return ESP_OK; }
inline void nvs_close(nvs_handle_t h) { nvs_sim::namespaces().erase(h); }

inline esp_err_t nvs_get_u8(nvs_handle_t h, const char *key, uint8_t *value) {
  size_t len = 1;
  return nvs_get_blob(h, key, value, &len) == ESP_OK && len == 1 ? ESP_OK : ESP_FAIL;
}
inline esp_err_t nvs_set_u8(nvs_handle_t h, const char *key, uint8_t value) { return nvs_set_blob(h, key, &value, 1); }
