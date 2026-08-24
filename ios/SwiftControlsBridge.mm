#include "Simulator-Bridging-Header.h"

#include <BoardConfig.h>

#include "SimulatorInput.h"

extern "C" bool simControlHasTouch(void) { return BoardConfig::hasTouch(); }

extern "C" bool simControlHasHomeKey(void) {
  return BoardConfig::hasHomeKey();
}

extern "C" void simControlSetButton(const int button, const bool down) {
  down ? SimulatorInput::buttonDown(button) : SimulatorInput::buttonUp(button);
}

extern "C" void simControlSetHomeKey(const bool down) {
  SimulatorInput::homeKey(down);
}
