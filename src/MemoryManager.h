#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>

#include "esp_heap_caps.h"

namespace freeink {

enum class MemPool : uint8_t { Internal, Psram, Default };

struct CacheSink {
  const char* name = nullptr;
  uint8_t priority = 128;
  std::function<size_t(size_t bytesRequested)> evict;
};

class MemoryManager {
 public:
  static constexpr int kMaxSinks = 12;

  static MemoryManager& instance() {
    static MemoryManager manager;
    return manager;
  }

  int registerSink(const CacheSink& sink) {
    if (!sink.evict) return -1;
    for (int i = 0; i < kMaxSinks; ++i) {
      if (sinks_[i].used && sinks_[i].sink.name && sink.name && std::strcmp(sinks_[i].sink.name, sink.name) == 0) {
        sinks_[i].sink = sink;
        return sinks_[i].id;
      }
    }
    for (int i = 0; i < kMaxSinks; ++i) {
      if (!sinks_[i].used) {
        sinks_[i] = {sink, nextId_++, true};
        return sinks_[i].id;
      }
    }
    return -1;
  }

  size_t freeBytes(const MemPool pool = MemPool::Default) const { return heap_caps_get_free_size(capsFor(pool)); }

  size_t clearCaches(const size_t bytesTarget = 0) {
    size_t freed = 0;
    bool visited[kMaxSinks] = {};
    for (;;) {
      int selected = -1;
      for (int i = 0; i < kMaxSinks; ++i) {
        if (!sinks_[i].used || visited[i]) continue;
        if (selected < 0 || sinks_[i].sink.priority < sinks_[selected].sink.priority) selected = i;
      }
      if (selected < 0 || (bytesTarget != 0 && freed >= bytesTarget)) return freed;
      visited[selected] = true;
      freed += sinks_[selected].sink.evict(bytesTarget == 0 ? 0 : bytesTarget - freed);
    }
  }

  bool ensureFree(const size_t bytes, const MemPool pool = MemPool::Default) {
    if (freeBytes(pool) >= bytes) return true;
    clearCaches(bytes - freeBytes(pool));
    return freeBytes(pool) >= bytes;
  }

 private:
  struct Entry {
    CacheSink sink;
    int id = 0;
    bool used = false;
  };

  static unsigned capsFor(const MemPool pool) {
    switch (pool) {
      case MemPool::Internal: return MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT;
      case MemPool::Psram: return MALLOC_CAP_SPIRAM;
      case MemPool::Default: return MALLOC_CAP_DEFAULT;
    }
    return MALLOC_CAP_DEFAULT;
  }

  Entry sinks_[kMaxSinks];
  int nextId_ = 1;
};

}  // namespace freeink
