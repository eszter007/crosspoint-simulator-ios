#include <TrustedTime.h>

#include <ctime>

// Simulator stand-in: the host clock is always trustworthy, nothing persists.
namespace trustedtime {
void init() {}
void note() {}
void startSync() {}
bool syncNow(uint32_t) { return true; }
int64_t trustedNow() { return static_cast<int64_t>(time(nullptr)); }
}  // namespace trustedtime
