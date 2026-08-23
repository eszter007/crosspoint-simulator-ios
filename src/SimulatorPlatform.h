#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Absolute path of the writable document store the simulated SD card lives in,
// or NULL to use the simulator's default (CROSSPOINT_SIM_SD, else ./fs_).
// On iOS this is the app's Documents directory -- the only writable location it
// has, and the one the Files app shows.
const char *simPlatformDocumentsPath(void);

// Ask the platform to let the user choose the folder the simulated SD card
// lives in. No-op where there is nothing to pick. Asynchronous: the storage
// root changes once the user has chosen, and the simulator re-reads it on the
// next path resolution, so no restart is needed.
void simPlatformPickFolder(void);

// True when the platform can present a folder picker, so the simulator knows
// whether to offer the control at all.
bool simPlatformCanPickFolder(void);

#ifdef __cplusplus
}
#endif
