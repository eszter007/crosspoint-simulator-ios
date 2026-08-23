#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Absolute path of the writable document store the simulated SD card lives in,
// or NULL to use the simulator's default (CROSSPOINT_SIM_SD, else ./fs_).
// On iOS this is the app's Documents directory -- the only writable location it
// has, and the one the Files app shows.
const char *simPlatformDocumentsPath(void);

#ifdef __cplusplus
}
#endif
