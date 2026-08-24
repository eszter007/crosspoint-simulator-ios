#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

bool simControlHasTouch(void);
bool simControlHasHomeKey(void);
void simControlSetButton(int button, bool down);
void simControlSetHomeKey(bool down);
void simPlatformPickFolder(void);

#ifdef __cplusplus
}
#endif
