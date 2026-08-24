
#include <SDL.h>
// Renames main() to SDL_main on the platforms that need SDL to own the real
// entry point -- iOS, where SDL2main stands up UIApplication first. Inert
// elsewhere.
#include <SDL_main.h>
#include <unistd.h>

#include "Arduino.h"
#include "HalDisplay.h"
#include "HalGPIO.h"
#include "SimulatorLifecycle.h"

extern void setup();
extern void loop();
extern HalDisplay display; // defined in main.cpp

int main(int argc, char **argv) {
  SimulatorLifecycle::initProcessArgs(argv);
  setup();
  while (!display.shouldQuit()) {
    // Backgrounded on iOS: block instead of running the firmware and drawing.
    // A background app that draws or burns CPU is one iOS terminates. The
    // wait also parks the loop until something wakes it, which is the closest
    // analogue to the device being asleep.
    if (gpio.isBackgrounded()) {
      SDL_WaitEventTimeout(nullptr, 200);
      continue;
    }
    // Clear input edge latches once per frame. update() may be called many
    // times within loop(); edges must survive across those calls and only
    // reset here at the frame boundary.
    gpio.beginFrame();
    loop();
    // SDL must be driven from the main thread on macOS.
    // The render task writes pixels and sets pendingPresent; we flush them
    // here.
    display.presentIfNeeded();
    // Yield to the OS so it can deliver pending input and window events.
#if defined(SIMULATOR_IOS)
    SDL_Delay(16);
#else
    SDL_Delay(1);
#endif
  }
  SDL_Quit();
  // Use _exit() instead of return/exit() to bypass C++ global destructors.
  // `activityManager` (and other globals in main.cpp) are constructed before
  // the render task thread starts, and the render task runs a [[noreturn]]
  // infinite loop.  If normal exit() runs global destructors while the render
  // thread is mid-render, the destructor races with the thread → SIGABRT/
  // SIGSEGV → "quit unexpectedly" dialog.  SDL is already torn down above, so
  // calling _exit(0) here is safe.
  _exit(0);
}
