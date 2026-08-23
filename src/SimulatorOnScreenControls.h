#pragma once

#include <SDL.h>

// On-screen hardware keys, drawn in a strip below the panel.
//
// The simulator's keys are normally the host keyboard. A phone has none, and on
// a touch board the panel itself is already an input surface, so without this
// the X4 Pro's page keys, Power and capacitive Home key are unreachable. The
// strip draws whatever keys the simulated board actually has, from BoardConfig.
//
// Always on for iOS. Elsewhere it is opt-in with CROSSPOINT_SIM_CONTROLS=1, so
// the window stays exactly panel-sized by default.
namespace SimulatorOnScreenControls {

bool enabled();

// Height in logical pixels the strip adds below the panel, or 0 when disabled
// or when the board has no keys to show.
int stripHeight();

// Draw into the strip. Called after the panel has been copied, before present.
void render(SDL_Renderer *renderer, int logicalWidth, int logicalHeight);

// Route a press/release at a logical point. Returns true when the strip
// consumed it, in which case it must NOT also be treated as a panel touch.
bool handlePress(int logicalX, int logicalY);
bool handleRelease();

// True while a strip key is held, so a drag off it does not leak into a panel
// touch.
bool capturing();

} // namespace SimulatorOnScreenControls
