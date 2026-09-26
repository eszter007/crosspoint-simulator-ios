// Host implementation of the one MemoryManager entry point the firmware reaches on this build.
//
// The SDK's own MemoryManager.cpp is device code: it reads ESP-IDF heap capabilities and parks a
// FreeRTOS static task, neither of which exists here. WordStore (the paragraph word arena) calls
// ensureFree() before growing a chunk, so only that is needed.
//
// On a host the answer is always yes: there are no registered cache sinks to evict and the
// process heap is not the ~380KB the device budgets against. Returning true means the arena
// grows as asked, which is the behaviour a device with headroom also shows.
#include <MemoryManager.h>

#include <Arduino.h>

namespace freeink {

MemoryManager &MemoryManager::instance() {
  static MemoryManager manager;
  return manager;
}

// Nothing to evict here, so registration is accepted and ignored. A real handle id keeps callers
// that check for -1 (table full) on their success path.
int MemoryManager::registerSink(const CacheSink &) { return 0; }

bool MemoryManager::ensureFree(size_t, MemPool) { return true; }

// Answer from the same figures every other heap query in the simulator uses, so the firmware's
// low-memory branches stay drivable through CROSSPOINT_SIM_FREE_HEAP.
size_t MemoryManager::freeBytes(MemPool) const { return ESP.getFreeHeap(); }

}  // namespace freeink
