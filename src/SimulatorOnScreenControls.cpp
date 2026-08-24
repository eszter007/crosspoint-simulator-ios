#include "SimulatorOnScreenControls.h"

#include <BoardConfig.h>
#include <HalDisplay.h>
#include <HalGPIO.h>

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "SimulatorInput.h"
#include "SimulatorPlatform.h"

extern HalDisplay display; // defined in the firmware's main.cpp

namespace SimulatorOnScreenControls {
namespace {

// HOME_KEY is not a HalGPIO button: the capacitive Home key is reported by the
// touch controller, so it goes in through SimulatorInput::homeKey().
constexpr int HOME_KEY = -2;
// Not a device key at all: the simulator's own control for choosing which
// folder the simulated SD card lives in. Only offered where the platform has a
// folder picker (iOS), and drawn set apart from the hardware keys.
constexpr int PICK_FOLDER = -3;

enum class Glyph { ChevronUp, ChevronDown, ChevronLeft, ChevronRight, Confirm, Back, Power, Home, Folder };

struct Control {
  int button;
  Glyph glyph;
  SDL_Rect rect{};
};

Control controls[8];
int controlCount = 0;
bool built = false;
int captured = -1; // index into controls, or -1

void add(const int button, const Glyph glyph) {
  if (controlCount >= static_cast<int>(sizeof(controls) / sizeof(controls[0])))
    return;
  controls[controlCount].button = button;
  controls[controlCount].glyph = glyph;
  controlCount++;
}

// Built from the active board profile, so the strip offers the keys that board
// really has. The simulator's BoardConfig models the page-key pair and the
// device's capabilities rather than a pin per button, so the split is by
// capability:
//
//   Touch boards (X4 Pro, Sticky, Paper Mono) reach Back and Confirm through
//   the panel, exactly as the hardware does, so the strip carries only what the
//   panel cannot provide: the page keys, Power, and the capacitive Home key.
//
//   Button boards (X4, X3) have no panel to press, so the strip stands in for
//   the whole keyboard mapping.
//
// Left to right in the order a thumb reaches them.
void build() {
  if (built)
    return;
  built = true;

  if (simPlatformCanPickFolder()) {
    add(PICK_FOLDER, Glyph::Folder);
  }

  if (BoardConfig::hasTouch()) {
    add(HalGPIO::BTN_UP, Glyph::ChevronUp);
    if (BoardConfig::hasHomeKey())
      add(HOME_KEY, Glyph::Home);
    add(HalGPIO::BTN_POWER, Glyph::Power);
    add(HalGPIO::BTN_DOWN, Glyph::ChevronDown);
    return;
  }

  add(HalGPIO::BTN_BACK, Glyph::Back);
  add(HalGPIO::BTN_LEFT, Glyph::ChevronLeft);
  add(HalGPIO::BTN_UP, Glyph::ChevronUp);
  add(HalGPIO::BTN_CONFIRM, Glyph::Confirm);
  add(HalGPIO::BTN_DOWN, Glyph::ChevronDown);
  add(HalGPIO::BTN_RIGHT, Glyph::ChevronRight);
  add(HalGPIO::BTN_POWER, Glyph::Power);
}

void drawThickLine(SDL_Renderer *r, int x1, int y1, int x2, int y2, int thickness) {
  const double dx = x2 - x1;
  const double dy = y2 - y1;
  const double len = std::sqrt(dx * dx + dy * dy);
  if (len < 0.5)
    return;
  // Offset along the line's own normal to give it width.
  const double nx = -dy / len;
  const double ny = dx / len;
  for (int t = -thickness / 2; t <= thickness / 2; t++) {
    const int ox = static_cast<int>(std::lround(nx * t));
    const int oy = static_cast<int>(std::lround(ny * t));
    SDL_RenderDrawLine(r, x1 + ox, y1 + oy, x2 + ox, y2 + oy);
  }
}

void drawArc(SDL_Renderer *r, int cx, int cy, int radius, double fromDeg, double toDeg, int thickness) {
  // Enough angular steps that neighbouring points land on adjacent pixels at
  // the outer radius, and half-pixel radial steps, or the stroke comes out
  // dotted rather than solid.
  const int steps = std::max(48, static_cast<int>((radius + thickness) * 8));
  const double half = thickness / 2.0;
  for (double t = -half; t <= half; t += 0.5) {
    const double rr = radius + t;
    if (rr <= 0)
      continue;
    for (int i = 0; i <= steps; i++) {
      const double a = (fromDeg + (toDeg - fromDeg) * i / steps) * M_PI / 180.0;
      SDL_RenderDrawPoint(r, cx + static_cast<int>(std::lround(std::cos(a) * rr)),
                          cy + static_cast<int>(std::lround(std::sin(a) * rr)));
    }
  }
}

void drawGlyph(SDL_Renderer *r, const Glyph glyph, const SDL_Rect &rect) {
  const int cx = rect.x + rect.w / 2;
  const int cy = rect.y + rect.h / 2;
  const int s = std::max(4, rect.w / 4);
  const int th = std::max(2, rect.w / 14);

  switch (glyph) {
  case Glyph::ChevronUp:
    drawThickLine(r, cx - s, cy + s / 2, cx, cy - s / 2, th);
    drawThickLine(r, cx, cy - s / 2, cx + s, cy + s / 2, th);
    break;
  case Glyph::ChevronDown:
    drawThickLine(r, cx - s, cy - s / 2, cx, cy + s / 2, th);
    drawThickLine(r, cx, cy + s / 2, cx + s, cy - s / 2, th);
    break;
  case Glyph::ChevronLeft:
    drawThickLine(r, cx + s / 2, cy - s, cx - s / 2, cy, th);
    drawThickLine(r, cx - s / 2, cy, cx + s / 2, cy + s, th);
    break;
  case Glyph::ChevronRight:
    drawThickLine(r, cx - s / 2, cy - s, cx + s / 2, cy, th);
    drawThickLine(r, cx + s / 2, cy, cx - s / 2, cy + s, th);
    break;
  case Glyph::Confirm:
    drawThickLine(r, cx - s, cy, cx - s / 3, cy + s * 2 / 3, th);
    drawThickLine(r, cx - s / 3, cy + s * 2 / 3, cx + s, cy - s * 2 / 3, th);
    break;
  case Glyph::Back:
    // Arrow, not a bare chevron: a board with both Back and Left would
    // otherwise show the same glyph twice.
    drawThickLine(r, cx - s, cy, cx + s, cy, th);
    drawThickLine(r, cx - s, cy, cx - s / 4, cy - s * 3 / 4, th);
    drawThickLine(r, cx - s, cy, cx - s / 4, cy + s * 3 / 4, th);
    break;
  case Glyph::Power:
    drawArc(r, cx, cy, s, -60, 240, th);
    drawThickLine(r, cx, cy - s - th, cx, cy - s / 3, th);
    break;
  case Glyph::Home:
    drawArc(r, cx, cy, s, 0, 360, th);
    break;
  case Glyph::Folder:
    // Folder outline: back edge, tab, and body.
    drawThickLine(r, cx - s, cy + s * 2 / 3, cx - s, cy - s * 2 / 3, th);
    drawThickLine(r, cx - s, cy - s * 2 / 3, cx - s / 6, cy - s * 2 / 3, th);
    drawThickLine(r, cx - s / 6, cy - s * 2 / 3, cx + s / 6, cy - s / 3, th);
    drawThickLine(r, cx + s / 6, cy - s / 3, cx + s, cy - s / 3, th);
    drawThickLine(r, cx + s, cy - s / 3, cx + s, cy + s * 2 / 3, th);
    drawThickLine(r, cx + s, cy + s * 2 / 3, cx - s, cy + s * 2 / 3, th);
    break;
  }
}

bool isDown(const Control &c) {
  // The two non-device controls have no button state to read; they look pressed
  // only while the finger is on them.
  if (c.button < 0)
    return captured >= 0 && controls[captured].button == c.button;
  return gpio.isPressed(static_cast<uint8_t>(c.button));
}

} // namespace

bool enabled() {
#if defined(SIMULATOR_IOS)
  return false; // SwiftUI supplies the iPhone controls.
#else
  static const bool on = std::getenv("CROSSPOINT_SIM_CONTROLS") != nullptr;
  return on;
#endif
}

// Logical pixels reserved for the keys themselves.
constexpr int KEY_AREA_HEIGHT = 96;
// Extra logical pixels below them on iOS, clearing the home indicator, which
// the system draws over the bottom of the screen. A constant rather than the
// measured safeAreaInsets: the panel and strip are letterboxed by SDL, so the
// inset would have to be converted through the current scale, and none of that
// could be checked without a device. 40 covers the largest indicator inset
// (34pt) on every current iPhone with room to spare.
constexpr int IOS_BOTTOM_GUTTER = 40;

int stripHeight() {
  if (!enabled())
    return 0;
  build();
  if (controlCount == 0)
    return 0;
  // Fixed in logical pixels: the panel's logical size is fixed too, and SDL
  // scales the pair together to whatever the real screen is.
#if defined(SIMULATOR_IOS)
  return KEY_AREA_HEIGHT + IOS_BOTTOM_GUTTER;
#else
  return KEY_AREA_HEIGHT;
#endif
}

void render(SDL_Renderer *renderer, const int logicalWidth, const int logicalHeight) {
  const int strip = stripHeight();
  if (strip == 0 || renderer == nullptr)
    return;

  const int stripY = logicalHeight - strip;
  SDL_Rect background = {0, stripY, logicalWidth, strip};
  SDL_SetRenderDrawColor(renderer, 24, 24, 27, 255);
  SDL_RenderFillRect(renderer, &background);

  // Keys occupy the key area only; any gutter below stays empty so the home
  // indicator does not sit on top of one.
  const int keyAreaHeight = std::min(strip, KEY_AREA_HEIGHT);
  const int slotW = logicalWidth / controlCount;
  const int size = std::min(slotW - 12, keyAreaHeight - 20);
  for (int i = 0; i < controlCount; i++) {
    Control &c = controls[i];
    c.rect = {slotW * i + slotW / 2 - size / 2, stripY + keyAreaHeight / 2 - size / 2, size, size};

    // Pressed keys fill, idle keys are outlined: both read against the dark
    // strip without needing a font.
    if (isDown(c)) {
      SDL_SetRenderDrawColor(renderer, 90, 90, 96, 255);
      SDL_RenderFillRect(renderer, &c.rect);
      SDL_SetRenderDrawColor(renderer, 250, 250, 250, 255);
    } else {
      SDL_SetRenderDrawColor(renderer, 70, 70, 76, 255);
      SDL_RenderDrawRect(renderer, &c.rect);
      SDL_SetRenderDrawColor(renderer, 175, 175, 182, 255);
    }
    drawGlyph(renderer, c.glyph, c.rect);
  }
}

bool handlePress(const int logicalX, const int logicalY) {
  if (stripHeight() == 0)
    return false;
  for (int i = 0; i < controlCount; i++) {
    const SDL_Rect &r = controls[i].rect;
    if (r.w == 0)
      continue; // not laid out yet: nothing drawn, nothing to hit
    if (logicalX < r.x || logicalX >= r.x + r.w || logicalY < r.y || logicalY >= r.y + r.h)
      continue;
    captured = i;
    if (controls[i].button == HOME_KEY) {
      SimulatorInput::homeKey(true);
    } else if (controls[i].button != PICK_FOLDER) {
      SimulatorInput::buttonDown(controls[i].button);
    }
    display.requestPresent();
    return true;
  }
  return false;
}

bool handleRelease() {
  if (captured < 0)
    return false;
  const Control &c = controls[captured];
  if (c.button == HOME_KEY) {
    SimulatorInput::homeKey(false);
  } else if (c.button == PICK_FOLDER) {
    // On release, so a press that slides off does not open the picker.
    simPlatformPickFolder();
  } else {
    SimulatorInput::buttonUp(c.button);
  }
  captured = -1;
  display.requestPresent();
  return true;
}

bool capturing() { return captured >= 0; }

} // namespace SimulatorOnScreenControls
