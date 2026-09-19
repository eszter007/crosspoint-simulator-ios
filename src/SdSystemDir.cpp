#include "SdSystemDir.h"

#include <HalStorage.h>
#include <Logging.h>

namespace sdsystem {
namespace {
constexpr const char* HIDDEN = "/.system";
constexpr const char* VISIBLE = "/system";
const char* g_resolved = nullptr;
}  // namespace

const char* dir() {
  if (g_resolved) return g_resolved;
  // An existing folder wins, whichever spelling it uses, so an upgrade keeps reading the state it
  // already wrote. Hidden first: if a card somehow has both, the dotted one is the newer layout.
  if (Storage.exists(HIDDEN)) {
    g_resolved = HIDDEN;
  } else if (Storage.exists(VISIBLE)) {
    g_resolved = VISIBLE;
  } else {
    // Neither: create the hidden one. mkdir is a no-op when it loses a race.
    Storage.mkdir(HIDDEN);
    g_resolved = HIDDEN;
    LOG_INF("SDSYS", "Created %s for firmware state", HIDDEN);
  }
  return g_resolved;
}

std::string path(const char* leaf) {
  std::string out = dir();
  if (leaf && *leaf) {
    if (*leaf != '/') out += '/';
    out += leaf;
  }
  return out;
}

}  // namespace sdsystem
