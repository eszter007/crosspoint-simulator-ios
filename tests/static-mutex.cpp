#ifdef NDEBUG
#undef NDEBUG
#endif

#include <freertos/semphr.h>

#include <atomic>
#include <cassert>
#include <chrono>
#include <cstdio>
#include <thread>

int main() {
  StaticSemaphore_t storage;
  const auto mutex = xSemaphoreCreateMutexStatic(&storage);
  assert(mutex == &storage);
  assert(xSemaphoreCreateMutexStatic(nullptr) == nullptr);
  assert(xSemaphoreTake(mutex, portMAX_DELAY));
  assert(xSemaphoreGetMutexHolder(mutex) == xTaskGetCurrentTaskHandle());
  std::atomic<bool> started{false}, entered{false};
  std::thread worker([&] {
    started = true;
    assert(xSemaphoreTake(mutex, portMAX_DELAY));
    entered = true;
    assert(xSemaphoreGetMutexHolder(mutex) == xTaskGetCurrentTaskHandle());
    assert(xSemaphoreGive(mutex));
  });
  while (!started.load())
    std::this_thread::yield();
  std::this_thread::sleep_for(std::chrono::milliseconds(20));
  assert(!entered.load());
  assert(xSemaphoreGive(mutex));
  worker.join();
  assert(entered.load());
  assert(xSemaphoreGetMutexHolder(mutex) == nullptr);
  std::puts("Caller-owned static mutex excludes another thread and tracks its "
            "holder");
}
