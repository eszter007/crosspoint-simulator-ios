#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>

// Mirrors the firmware's HalMemory. The host has one ordinary heap, so the
// capability-specific views all answer from the same figures ESPMock reports
// (see Arduino.h) — that keeps CROSSPOINT_SIM_FREE_HEAP and
// CROSSPOINT_SIM_MAX_ALLOC_HEAP driving every heap query, whichever API the
// firmware happens to use, so the low-memory paths stay testable here.
class HalMemory {
public:
  struct HeapStats {
    size_t freeBytes;
    size_t totalBytes;
    size_t minFreeBytes;
    size_t largestBlockBytes;
  };

  static HeapStats getDefaultHeap();
  static HeapStats getInternalHeap();
  struct PsramDeleter {
    void operator()(uint8_t *buffer) const;
  };
  using PsramBuffer = std::unique_ptr<uint8_t[], PsramDeleter>;
  // The host has no PSRAM, and the contract is never to fall back to internal RAM, so this
  // always returns null. Callers treat that as "render uncached", which is the same path the
  // no-PSRAM ESP32-C3 boards take.
  static PsramBuffer allocatePsram(size_t bytes);

  static HeapStats getPsramHeap();
};
