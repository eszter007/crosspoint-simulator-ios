#pragma once

#include <cstddef>

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
  static HeapStats getPsramHeap();
};
