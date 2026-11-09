#pragma once

#include <cstdlib>
#include <string>

#include "SimulatorPlatform.h"

namespace crosspoint_simulator {

// Host folder that stands in for the SD card.
inline std::string storageRoot() {
  const char *root = std::getenv("CROSSPOINT_SIM_SD");
  if (!root || !*root) {
    root = std::getenv("CROSSPOINT_EMU_SD");
  }
  if (root && *root) {
    return std::string(root);
  }
  // Platforms with no usable working directory name their own location; on iOS
  // that is the app's sandboxed Documents directory (or the folder the user
  // picked). Still overridable above, so a desktop run is unaffected.
  if (const char *documents = simPlatformDocumentsPath()) {
    return std::string(documents);
  }
  return std::string("./fs_");
}

} // namespace crosspoint_simulator
