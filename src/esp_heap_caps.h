#pragma once

#include <cstddef>
#include <cstdint>

#include "Arduino.h"

// ESP-IDF's capability-aware heap queries, as far as the firmware uses them: telemetry in the
// SD font loader. The host has one flat heap, so every capability answers with the same numbers
// ESPMock reports -- which keeps CROSSPOINT_SIM_FREE_HEAP / _MAX_ALLOC_HEAP steering these too.
#define MALLOC_CAP_EXEC (1 << 0)
#define MALLOC_CAP_32BIT (1 << 1)
#define MALLOC_CAP_8BIT (1 << 2)
#define MALLOC_CAP_DMA (1 << 3)
#define MALLOC_CAP_SPIRAM (1 << 10)
#define MALLOC_CAP_INTERNAL (1 << 11)
#define MALLOC_CAP_DEFAULT (1 << 12)

// SPIRAM answers zero, matching HalMemory::getPsramHeap(). The TTF path asks whether PSRAM can
// hold a face before choosing between a resident and a streamed one, and whether to gate the UI
// fallbacks on internal heap; reporting the host heap for SPIRAM would make the host look like a
// PSRAM board and take branches no simulated device takes.
inline bool heap_caps_wants_psram(uint32_t caps) { return (caps & MALLOC_CAP_SPIRAM) != 0; }
inline size_t heap_caps_get_free_size(uint32_t caps) {
  return heap_caps_wants_psram(caps) ? 0 : ESP.getFreeHeap();
}
inline size_t heap_caps_get_largest_free_block(uint32_t caps) {
  return heap_caps_wants_psram(caps) ? 0 : ESP.getMaxAllocHeap();
}
inline size_t heap_caps_get_minimum_free_size(uint32_t /*caps*/) { return ESP.getMinFreeHeap(); }
inline size_t heap_caps_get_total_size(uint32_t /*caps*/) { return ESP.getHeapSize(); }
