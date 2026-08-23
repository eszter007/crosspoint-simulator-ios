#pragma once

// Live input injection, for the on-screen key strip.
//
// Distinct from the scripted synthetic events in HalGPIO.cpp, which are queued
// up front with fire times. These arrive from the user mid-run and land on the
// same latches a keyboard press would.
namespace SimulatorInput {

// buttonIndex is a HalGPIO::BTN_* value.
void buttonDown(int buttonIndex);
void buttonUp(int buttonIndex);
void homeKey(bool down);

// Called once per frame from HalGPIO::beginFrame(), before the edge latches are
// cleared. Releases a key whose press landed in the same frame, so a very fast
// tap still spans a press frame and a release frame rather than collapsing into
// one with zero held time.
void tick();

} // namespace SimulatorInput
