#include "HalMemory.h"

#include "Arduino.h"

#include <atomic>
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <new>

namespace {
// Each PSRAM block carries its size in front, so the deleter can return it to the budget.
constexpr size_t kPsramHeader = 16;

size_t psramTotal() {
  static const size_t total = [] {
    const char *value = std::getenv("CROSSPOINT_SIM_PSRAM");
    if (!value || *value == '\0') return size_t{0};
    char *end = nullptr;
    errno = 0;
    const unsigned long long parsed = std::strtoull(value, &end, 10);
    return errno == ERANGE || *end != '\0' ? size_t{0} : static_cast<size_t>(parsed);
  }();
  return total;
}

std::atomic<size_t> psramUsed{0};

HalMemory::HeapStats hostHeap() {
  return {ESP.getFreeHeap(), ESP.getHeapSize(), ESP.getMinFreeHeap(),
          ESP.getMaxAllocHeap()};
}
} // namespace

HalMemory::HeapStats HalMemory::getDefaultHeap() { return hostHeap(); }

HalMemory::HeapStats HalMemory::getInternalHeap() { return hostHeap(); }

// No PSRAM unless CROSSPOINT_SIM_PSRAM sets one: reporting a pool that does not exist would make
// the firmware's logs lie about where memory went, and take branches no simulated device takes.
HalMemory::HeapStats HalMemory::getPsramHeap() {
  const size_t total = psramTotal();
  const size_t used = psramUsed.load();
  const size_t free = total > used ? total - used : 0;
  return {free, total, free, free};
}

void HalMemory::PsramDeleter::operator()(uint8_t *buffer) const {
  if (!buffer) return;
  uint8_t *base = buffer - kPsramHeader;
  size_t bytes = 0;
  memcpy(&bytes, base, sizeof(bytes));
  psramUsed -= bytes;
  delete[] base;
}

HalMemory::PsramBuffer HalMemory::allocatePsram(size_t bytes) {
  const size_t used = psramUsed.load();
  if (psramTotal() == 0 || bytes > psramTotal() - std::min(used, psramTotal())) return {};
  auto *base = new (std::nothrow) uint8_t[bytes + kPsramHeader];
  if (!base) return {};
  memcpy(base, &bytes, sizeof(bytes));
  psramUsed += bytes;
  return PsramBuffer(base + kPsramHeader);
}
