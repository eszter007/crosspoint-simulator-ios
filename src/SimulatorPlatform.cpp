#include "SimulatorPlatform.h"

// Desktop implementation. iOS supplies its own from ios/sim_ios_platform.m,
// which the CMake build swaps in for this file; the PlatformIO build only ever
// compiles this one.
#if !defined(SIMULATOR_IOS)

extern "C" const char *simPlatformDocumentsPath(void) { return nullptr; }

// Desktop already has a working directory and CROSSPOINT_SIM_SD; there is
// nothing a picker would add.
extern "C" void simPlatformPickFolder(void) {}
extern "C" bool simPlatformCanPickFolder(void) { return false; }

#endif
