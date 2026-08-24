# Running the CrossPoint simulator on an iPhone

The simulator builds as an iOS app: the same firmware sources, the same board
profile, driven by your finger instead of a keyboard. It is the natural home for
the **X4 Pro**, whose real input is a touch panel and a capacitive Home key.

Everything here needs a Mac with Xcode. There is no way to build or run an iOS
app from Linux or Windows.

## Why this directory exists

The simulator normally builds through PlatformIO, as a library the firmware
consumes via `lib_deps` (see the repository root's `sample-platformio-*.ini`).
PlatformIO has no iOS platform, so an iPhone build needs its own build system.
`ios/CMakeLists.txt` compiles exactly what the PlatformIO env does — same
sources, same flags, same excluded files — and additionally knows how to produce
an app bundle.

The PlatformIO path is untouched. Both build the same simulator.

## What you need

- macOS with Xcode 15+ and the iOS SDK
- CMake 3.16+ and Python 3
- An **SDL2** source checkout (not SDL3, and not a Homebrew install — iOS needs
  SDL compiled for the device):

  ```sh
  git clone --branch SDL2 https://github.com/libsdl-org/SDL.git ~/src/SDL2
  ```

- The firmware, with its submodules:

  ```sh
  git clone --recurse-submodules https://github.com/crosspoint-reader/crosspoint-reader.git
  ```

- An Apple developer account. A free one is enough to run on your own device.

## Build

```sh
cmake -S ios -B build-ios -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCROSSPOINT_FIRMWARE_ROOT=../crosspoint-reader \
      -DSDL2_SOURCE_DIR=$HOME/src/SDL2
open build-ios/crosspoint-sim-ios.xcodeproj
```

In Xcode, pick your device, set a signing team on the `crosspoint_simulator`
target (Signing & Capabilities — CMake cannot generate one), and Run.

`-DSIMULATOR_DEVICE=` selects the board, defaulting to `x4pro`. The other values
are `x4`, `x3`, `sticky` and `papermono`, matching the PlatformIO envs;
`-DSIMULATOR_DISPLAY=uc8179|uc8279` overrides the panel controller.

## Building for the desktop with CMake

The same CMakeLists produces a native binary, which is how the iOS-bound changes
are checked without a device:

```sh
cmake -S ios -B build -DCROSSPOINT_FIRMWARE_ROOT=../crosspoint-reader
cmake --build build
CROSSPOINT_SIM_SD=./fs_ CROSSPOINT_SIM_CONTROLS=1 ./build/crosspoint_simulator
```

This is a convenience, not a replacement: PlatformIO remains the supported
desktop path.

## Firmware forks

The simulator replaces the firmware's HAL, so it is tied to one firmware's HAL
by construction — see [FORKING.md](../FORKING.md). This fork additionally builds
against [matcha-reader](https://github.com/eszter007/matcha-reader), a
Japanese-enabled CrossPoint fork:

```sh
cmake -S ios -B build-matcha -DCROSSPOINT_FIRMWARE_ROOT=../matcha-reader
cmake --build build-matcha
```

What that needed, for reference if you carry another fork:

- **Matcha's HAL additions**, stubbed to match its signatures:
  `HalGPIO::anyButtonDownRaw`, `HalClock::systemTimeValid` /
  `restoreSystemTime` / `persistSystemTime` / `localEpoch`, and
  `HalFile::modifiedStamp`.
- **Gaps in the platform emulation layer**, which are not fork-specific and
  would be worth sending upstream: `pdPASS`, `xTaskNotifyGive`,
  `ulTaskNotifyValueClear`, `vSemaphoreDelete`, the `JPEGDEC` result enum, and
  `SecureHttpClient`'s `std::string` `POST`/`getString` signatures, which had
  drifted from the SDK's.

Both firmwares build and run from the same tree; upstream is checked on every
change here.

## Getting books onto it

Two routes, and one that does not work — see below on File Transfer.

### 1. Drop them in the Files app (default, nothing to configure)

The app's Documents directory *is* the SD card. `UIFileSharingEnabled` and
`LSSupportsOpeningDocumentsInPlace` are set, so it appears in the Files app under
**On My iPhone → CrossPoint**, and in Finder's Files tab when the phone is
plugged in. Drop `.epub` files straight in — a whole folder of them at once is
fine.

### 2. Point it at a folder you already have

The **folder key** at the left of the on-screen strip opens the system folder
picker. Choose any folder Files can reach — iCloud Drive, a USB drive, another
app's shared folder — and the simulated SD card becomes that folder, in place.
Nothing is copied.

The choice is kept as a security-scoped bookmark, so it survives relaunches
without asking again. It takes effect immediately: the storage root is re-read
on every path resolution, so no restart is needed, though the library screen
needs revisiting to re-scan. If the folder later disappears (deleted, or on a
drive that is no longer attached) the simulator falls back to Documents.

`CROSSPOINT_SIM_SD` still wins over both where it is set, so desktop runs are
unaffected. The folder key only appears where the platform has a picker, so the
desktop strip is unchanged.

### Why not File Transfer?

The firmware's File Transfer screen runs a web server, and it does start under
iOS — in the simulator it runs on its own thread rather than being driven by the
firmware loop, so it is not affected by the app pausing in the background.

It is still not a usable route on a phone. The server binds to loopback only
(`127.0.0.1`), so nothing else on the network can reach it; the only browser
that can is Safari on the same phone, and switching to Safari backgrounds the
simulator, which iOS suspends within seconds. Reaching it from a laptop would
need the server bound to all interfaces and an `NSLocalNetworkUsageDescription`
prompt, which is a change to the simulator's deliberate loopback sandbox rather
than an iOS detail.

The Files app and the folder picker both do the job without any of that.

## On-screen keys

A phone has no keyboard, and on a touch board the panel is already an input
surface — so the simulator places native SwiftUI buttons with SF Symbols below
the centered panel. What appears comes from the board profile:

- **Touch boards** (X4 Pro, Sticky, Paper Mono) reach Back and Confirm through
  the panel, exactly as the hardware does. The strip carries only what the panel
  cannot provide: the two page keys, Power, and — on the X4 Pro — the capacitive
  Home key.
- **Button boards** (X4, X3) have no panel to press, so the strip stands in for
  the whole keyboard mapping.

The SwiftUI controls are always on for iOS. On desktop the SDL strip is opt-in with
`CROSSPOINT_SIM_CONTROLS=1`, so the window stays exactly panel-sized by default.

## Differences from the device

- **Multi-touch** is not simulated; nothing in the firmware's HAL exposes it.
- **No translation or OTA.** The firmware-update paths are excluded from the
  simulator build, and iOS has no linkable libcurl, so the desktop simulator's
  HTTP client is not built there either.
- **Wall-clock speed.** E-ink refresh timing is not simulated: pages appear
  instantly rather than taking the panel's 1–2 s.
- **Controls live outside SDL.** SwiftUI positions them against the measured
  safe area while SDL keeps the panel centered in the iPhone view.

## If the build fails

- **`iOS builds need -DSDL2_SOURCE_DIR=...`** — the path must point at an SDL2
  checkout's top level (the directory holding its `CMakeLists.txt`).
- **`No FreeInk SDK at ...`** — the firmware's submodules are missing:
  `git -C <firmware> submodule update --init --recursive`.
- **Signing errors on Run** — set a team on the target. Xcode will not run an
  unsigned app on a device.
- **New files not picked up** — re-run `cmake`; the Xcode generator writes the
  project once from the source lists.
