#include "HalMemory.h"

#include "Arduino.h"

namespace {
HalMemory::HeapStats hostHeap() {
  return {ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap(),
          ESP.getMaxAllocHeap()};
}
} // namespace

HalMemory::HeapStats HalMemory::getDefaultHeap() { return hostHeap(); }

HalMemory::HeapStats HalMemory::getInternalHeap() { return hostHeap(); }

// No PSRAM on the host. Firmware only logs these, and reporting a second pool
// that does not exist would make those lines lie about where memory went.
HalMemory::HeapStats HalMemory::getPsramHeap() { return {0, 0, 0, 0}; }
