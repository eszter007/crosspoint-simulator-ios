# CrossPoint Simulator — with an iPhone build

A fork of [crosspoint-reader/crosspoint-simulator](https://github.com/crosspoint-reader/crosspoint-simulator)
that additionally builds as an **iOS app**, so the simulator runs on an iPhone
with the panel taking real touch input. It is aimed at the **Xteink X4 Pro**,
whose real input is a touch panel and a capacitive Home key.

The iPhone build is what this fork is for, so it is what this README covers.
Everything upstream does is unchanged and still works exactly as before — the
PlatformIO desktop path, the device and panel-controller envs, and the
simulator's own HAL — and it is documented at the bottom, under
[Upstream: the desktop simulator](#upstream-the-desktop-simulator).

The simulator builds as an iOS app from the same firmware sources and the same
board profile, driven by your finger instead of a keyboard. Everything here
needs a Mac with Xcode; there is no way to build or run an iOS app from Linux or
Windows.

## Why the iOS build is separate

The simulator normally builds through PlatformIO, as a library the firmware
consumes via `lib_deps` (see `sample-platformio-*.ini`). PlatformIO has no iOS
platform, so an iPhone build needs its own build system. `ios/CMakeLists.txt`
compiles exactly what the PlatformIO env does — same sources, same flags, same
excluded files — and additionally knows how to produce an app bundle.

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

## Build for a device

```sh
cmake -S ios -B build-ios -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCROSSPOINT_FIRMWARE_ROOT="$PWD/../crosspoint-reader" \
      -DSDL2_SOURCE_DIR=$HOME/src/SDL2
open build-ios/crosspoint-sim-ios.xcodeproj
```

In Xcode, pick your device, set a signing team on the `crosspoint_simulator`
target (Signing & Capabilities — CMake cannot generate one), and Run.

> **Pass an absolute `CROSSPOINT_FIRMWARE_ROOT`.** A relative path is resolved
> against `ios/`, not the repository root, so `../crosspoint-reader` looks for
> the firmware *inside* this repo and fails with "No firmware at …".

`-DSIMULATOR_DEVICE=` selects the board, defaulting to `x4pro`. The other values
are `x4`, `x3`, `sticky` and `papermono`, matching the PlatformIO envs;
`-DSIMULATOR_DISPLAY=uc8179|uc8279` overrides the panel controller.

### Without opening Xcode

The same project builds, installs and launches from the command line, which is
the quicker loop once signing works:

```sh
xcodebuild -project build-ios/crosspoint-sim-ios.xcodeproj \
           -scheme crosspoint_simulator -configuration Debug \
           -destination 'generic/platform=iOS' \
           DEVELOPMENT_TEAM=XXXXXXXXXX CODE_SIGN_STYLE=Automatic build

xcrun devicectl list devices                 # find your device's UDID
xcrun devicectl device install app --device <UDID> \
      build-ios/Debug-iphoneos/crosspoint_simulator.app
xcrun devicectl device process launch --device <UDID> org.crosspoint.simulator
```

`DEVELOPMENT_TEAM` is the ten-character **team** id, which is not the id in your
signing certificate's name. Read it off the provisioning profile Xcode already
made rather than guessing:

```sh
security cms -D -i ~/Library/Developer/Xcode/UserData/Provisioning\ Profiles/*.mobileprovision \
  | plutil -p - | grep -A2 TeamIdentifier
```

The first launch on a given device fails with "its profile has not been
explicitly trusted by the user" — an install-time step Xcode prompts for and
`devicectl` does not. Trust it once under **Settings → General → VPN & Device
Management**, then launch again.

## Run it in the iOS Simulator

Useful for iterating without a phone, and the only way to drive the app
headlessly. It needs its own build directory: the device configuration above
pins the iPhoneOS SDK, and reusing it for a simulator build fails to link with
a page of undefined `_swift_*` symbols, because CMake has baked the device SDK's
Swift runtime path into the project.

```sh
cmake -S ios -B build-iossim -G Xcode \
      -DCMAKE_SYSTEM_NAME=iOS \
      -DCMAKE_OSX_SYSROOT=iphonesimulator \
      -DCMAKE_OSX_ARCHITECTURES="$(uname -m)" \
      -DCMAKE_XCODE_ATTRIBUTE_SUPPORTED_PLATFORMS=iphonesimulator \
      -DCROSSPOINT_FIRMWARE_ROOT="$PWD/../crosspoint-reader" \
      -DSDL2_SOURCE_DIR=$HOME/src/SDL2

xcodebuild -project build-iossim/crosspoint-sim-ios.xcodeproj \
           -scheme crosspoint_simulator -configuration Debug \
           -destination 'generic/platform=iOS Simulator' \
           CODE_SIGNING_ALLOWED=NO build

xcrun simctl boot 'iPhone 17 Pro'      # skip if one is already booted
open -a Simulator
xcrun simctl install booted build-iossim/Debug-iphonesimulator/crosspoint_simulator.app
xcrun simctl launch --console-pty booted org.crosspoint.simulator
```

Three details, each of which fails in a way that does not name its cause:

- **The architecture must match the host.** `$(uname -m)` gets it right; an
  arm64 bundle on an Intel Mac installs with "Failed to find matching arch"
  and the German-localised "must be updated by the developer" alert, even
  though the bundle itself is perfectly well formed.
- **Use the generic destination.** `xcodebuild` lists only "Any iOS Simulator
  Device" for this project and rejects a concrete simulator UDID with "Unable
  to find a destination matching the provided destination specifier", so tools
  that pass a specific simulator id cannot build it.
- **`--console-pty`** puts the firmware's own log on your terminal, which is
  where `[SIM]`, `[GFX]` and the firmware's `[DBG]` lines come out.

The simulated SD card is the app's Documents directory; find it with
`xcrun simctl get_app_container booted org.crosspoint.simulator data`, and drop
`.epub` files into it directly.

## Building for the desktop with CMake

The same CMakeLists produces a native binary, which is how the iOS-bound changes
are checked without a device:

```sh
cmake -S ios -B build -DCROSSPOINT_FIRMWARE_ROOT="$PWD/../crosspoint-reader"
cmake --build build
CROSSPOINT_SIM_SD=./fs_ CROSSPOINT_SIM_CONTROLS=1 ./build/crosspoint_simulator
```

This is a convenience, not a replacement: PlatformIO remains the supported
desktop path.

## Firmware forks

The simulator replaces the firmware's HAL, so it is tied to one firmware's HAL
by construction — see [FORKING.md](FORKING.md). This fork additionally builds
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
- **Newer HAL surface on Matcha's `develop`**, which is also upstream's
  direction rather than anything fork-specific: the `UsbDriveState` enum with
  `HalStorage::beginUsbDrive` / `disconnectUsbDriveHost` / `endUsbDrive` /
  `usbDriveState`, `HalStorage::prepareForDeepSleep`, and
  `BoardConfig::isX4Classic`. USB mass storage has no simulator counterpart —
  the SD card is a host directory, so `usbDriveState()` is always `Unsupported`
  and the firmware keeps to the path it already has for that. The USB Drive
  screen itself sits behind `FREEINK_CAP_USB_MSC` and is unreachable here; the
  stubs exist so the header compiles. `isX4Classic()` is always false: the SDK's
  X4 Classic is its own board and `SIMULATOR_DEVICE` has no profile for it.

All three build from the same tree — Matcha's merge branch and `develop`, and
upstream — and upstream is checked on every change here.

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

There is no "unpick" key. To go back to the built-in card, open the picker again
and choose the app's own Documents folder — it is in the picker as **On My
iPhone → CrossPoint**. Deleting the app also clears the bookmark, since it lives
in the app's defaults.

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

## Controls on the phone

The panel is letterboxed into the middle of the screen and a row of keys sits
below it, in the safe area. Between them they are the whole device: the panel is
the e-ink glass, the keys are the ones the hardware has that a touchscreen
cannot stand in for.

### The panel is the touch panel

On a touch board your finger on the panel *is* the finger on the glass, so the
firmware's own gestures are the controls — there is nothing simulator-specific
to learn:

- **Tap** a row, tile or tab to activate it. Tapping is also how you reach Back
  and Confirm, exactly as on the hardware.
- **Swipe up / down** to scroll a list or settings page, one screenful per
  swipe.
- **Swipe left / right** in the reader to turn pages.
- **Tap the status bar** — the top strip with the battery — from Home, the file
  browser or Settings to open the frontlight panel. A **swipe down from the top
  edge** does the same on any board that has a frontlight.
- **Drag** the brightness and warmth sliders in that panel; they track your
  finger.

Anything outside the panel is bezel: the black bands above and below it are not
part of the glass and swallow touches, so start an edge gesture just inside the
panel rather than at the very edge of the phone.

### The key strip

Native SwiftUI buttons with SF Symbols, sized to the 44pt minimum so they are
comfortable under a thumb. What appears comes from the board profile:

- **Touch boards** (X4 Pro, Sticky, Paper Mono) reach Back and Confirm through
  the panel, so the strip carries only what the panel cannot provide: the two
  page keys, Power, and — on the X4 Pro — the capacitive Home key. The X4 Pro
  strip reads: folder, page up, Home, Power, page down.
- **Button boards** (X4, X3) have no panel to press, so the strip stands in for
  the whole keyboard mapping: Back, Left, Up, Confirm, Down, Right, Power.

The page keys are worth knowing even on a touch board: they scroll a screen the
firmware has not given a swipe handler, and they step finely where a swipe
scrolls by a whole page.

The leftmost **folder key** is the one control with no hardware counterpart — it
opens the system folder picker to repoint the simulated SD card (see
[Getting books onto it](#getting-books-onto-it)).

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

---

## Upstream: the desktop simulator

Everything below is the upstream README, unchanged apart from heading levels:
the PlatformIO library that compiles the firmware natively and renders the
e-ink display in an SDL2 window on macOS or Linux. None of it is affected by
the iOS build above.

A desktop simulator for [CrossPoint](https://github.com/crosspoint-reader/crosspoint-reader)-based firmware. Compiles the firmware natively and renders the e-ink display in an SDL2 window. No device required. Can be used with forks of Crosspoint but any new methods added to the firmware will need to be stubbed. If your fork diverges from the CrossPoint HAL, see [FORKING.md](FORKING.md).

> [!NOTE]
> **Platform support:** macOS and Linux/WSL use different native compiler and library flags. Start from `sample-platformio-macos.ini` on macOS, or `sample-platformio-linux-wsl.ini` on Linux/WSL. Native Windows is not supported; use WSL and follow the Linux instructions.

> [!WARNING]
> This has been tested on x86_64 macOS (Intel), ARM64 macOS (Apple Silicon,
> M4), and Ubuntu under WSL on Windows. Other platforms may need additional
> libraries or platform-specific stubs.

### Prerequisites

SDL2 and `curl` must be installed on the host machine. Linux/WSL users also need OpenSSL development headers for MD5 support.

```bash
# macOS
brew install sdl2

# Linux — Debian/Ubuntu (including WSL)
sudo apt install libsdl2-dev libssl-dev

# Linux — Fedora/RHEL
sudo dnf install SDL2-devel openssl-devel

# Linux — Arch
sudo pacman -S sdl2 openssl
```

### Integration

Add the simulator to your firmware's platformio.ini as a `lib_dep` and configure the `[env:simulator]` environment. Use the sample file for your host OS:

- `sample-platformio-macos.ini`
- `sample-platformio-linux-wsl.ini`

No scripts need to be copied into the firmware repo for the simulator to build. The simulator library automatically patches consumer-side compatibility issues from its own build script when PlatformIO fetches it as a dependency, including the common `GfxRenderer::setOrientation()` hook needed for SDL window resizing.

Keep the sample `build_src_filter` exclusions unless your firmware has already
moved those files behind simulator guards. In the current CrossPoint layout,
the firmware-owned `CrossPointWebServer` and `WebDAVHandler` compile against
the simulator's lower-level `WebServer`, `WebSocketsServer`, and
`NetworkClient` shims. This exercises the real settings, files, status, and
WebDAV routes instead of a reduced simulator-only substitute.

The simulator defaults to the original X4 panel shape and SSD1677 controller.
Device-specific environments can extend the base simulator environment with
these flags:

- `-DSIMULATOR_DEVICE_X3` switches the framebuffer to 792x528 landscape,
  selects the X3 board profile, and exposes the simulator tilt sensor.
- `-DSIMULATOR_DEVICE_X4_PRO` keeps the X4 family's 800x480 framebuffer and
  selects the X4 Pro board profile. It exposes touch and swipe input, the
  capacitive Home key, the RTC, display inversion, and frontlight state.
- `-DSIMULATOR_DEVICE_STICKY` selects the Seeed Sticky's 800x480 SSD1677
  profile. It exposes touch and swipe input, the RTC, and the tilt sensor
  without exposing the X4 Pro-only Home key or frontlight.
- `-DSIMULATOR_DEVICE_PAPERMONO` selects the M5Stack PaperMono's 800x480
  SSD1677 profile. It exposes FT6336-compatible touch and swipe input, the RTC,
  and single-channel frontlight state without a Home key or color-temperature
  control.
- `-DSIMULATOR_DISPLAY_UC8179` selects the newer UC8179 controller used by
  some X4 and X4 Pro production batches.
- `-DSIMULATOR_DISPLAY_UC8279` selects UC8279d on X3, or the 800x480 UC8279
  controller on X4-family profiles.

The sample PlatformIO files include ready-to-use environments for the original
profiles plus `simulator_sticky`, `simulator_x3_uc8279`, `simulator_x4_uc8179`,
`simulator_x4_uc8279`, `simulator_x4_pro_uc8179`, and
`simulator_x4_pro_uc8279`, plus `simulator_papermono`. The UC8279 X4 Pro path
mirrors current FreeInk SDK support but remains pending validation on physical
UC8279 X4 Pro hardware.

Controller profiles expose the same framebuffer geometry and device
capabilities as their original production run. The simulator records the
selected `BoardConfig::DisplayController` and identifies it in the window title;
it does not attempt to model controller timing, LUT waveforms, ghosting, or
power sequencing.

If a fork has a custom renderer and the auto-patch cannot recognize it, its simulator build should notify the display when orientation changes:

```cpp
#ifdef SIMULATOR
display.setSimulatorOrientation(static_cast<int>(o));
#endif
```

Put that in the renderer's orientation setter after updating the renderer's own orientation state.
By default, the simulator keeps its own `JPEGDEC`, `PNGdec`, and QRCode compatibility shims so existing firmware projects can update this library without changing their simulator environment. To test against the native decoder libraries instead, follow the opt-in comments in the sample PlatformIO files: define `CROSSPOINT_SIM_USE_NATIVE_DECODERS`, set `lib_compat_mode = off`, change simulator `lib_ignore` to `hal, WebSockets`, and add the native `PNGdec`/`JPEGDEC` dependencies. `WebSockets` is ignored only in native simulator builds because this repo supplies the host-backed `WebSocketsServer` implementation.

If you only want a self-contained simulator dependency, stop there.

If you also want the `Run Simulator` task to appear in the consuming repo's PlatformIO IDE task list (under the "Custom" folder), let the consuming project own the IDE task registration. Add `custom_run_simulator_target_owner = project` to `[env:simulator]`, then add one project-level hook:

For a normal fetched dependency:

```ini
custom_run_simulator_target_owner = project

extra_scripts =
  pre:scripts/gen_i18n.py
  pre:scripts/git_branch.py
  pre:scripts/build_html.py
  post:.pio/libdeps/$PIOENV/simulator/run_simulator_project.py
```

For a local symlinked dependency:

```ini
custom_run_simulator_target_owner = project

extra_scripts =
  pre:scripts/gen_i18n.py
  pre:scripts/git_branch.py
  pre:scripts/build_html.py
  post:../crosspoint-simulator/run_simulator_project.py
```

Use the symlink form only when the `Crosspoint` repo and this `crosspoint-simulator` repo are checked out side by side and your `lib_deps` entry is:

```ini
simulator=symlink://../crosspoint-simulator
```

The `custom_run_simulator_target_owner = project` line tells the library-side hook not to register the same launcher a second time. Without that, closing one simulator window can immediately relaunch another because both the library hook and the project hook try to own `run_simulator`.

Do not point `post:` at `run_simulator.py` directly. That file is already auto-loaded via `library.json` and is the backward-compatible library hook.

The `post:` line above only exposes the task in the consuming project UI. The actual launcher logic still lives in this simulator repo.


### Setup

Place EPUB books at `./fs_/books/` in the Crosspoint repo's root. This maps to the `/books/` path on the physical SD card.

### Build and run

Run this command from the Crosspoint project after you have added the `[env:simulator]` config to Crosspoint's `platformio.ini` file. Alternatively, if you added the project hook above, you can click "Build" from PlatformIO's IDE task list and then "Run Simulator" (nested under the "Custom" folder).

```bash
pio run -e simulator -t run_simulator
```

### Controls

| Key    | Action                             |
| ------ | ---------------------------------- |
| ↑ / ↓  | Page back / forward (side buttons) |
| ← / →  | Left / right front buttons         |
| Return | Confirm / Select                   |
| Escape | Back                               |
| P      | Power                              |
| S      | Simulate sleep                     |
| H      | X4 Pro capacitive Home key         |
| Mouse  | Touch-device tap and swipe         |

When the simulator is on the sleep screen, pressing any mapped simulator key wakes it. Under the hood the simulator relaunches itself and reports a synthetic power-button wake, because the native build has no real ESP deep-sleep resume path.

### Automated QA and screenshots

Two optional environment variables make repeatable navigation and screenshot
tests possible without desktop-control permissions:

- `CROSSPOINT_SIM_INPUT_SCRIPT` schedules input as
  `<milliseconds>:<action>`, separated by semicolons. Button actions use
  `<key>[:<hold-milliseconds>]`; keys are `BACK`, `ENTER`, `LEFT`, `RIGHT`,
  `UP`, `DOWN`, `POWER`, `SLEEP`, `HOME`, and `QUIT`. A normal key press is
  held for 80 ms unless a duration is provided.
- Touch-device actions use `TAP:<x>,<y>[,<hold-milliseconds>]` or
  `SWIPE:<x1>,<y1>,<x2>,<y2>[,<duration-milliseconds>]`. Coordinates are in
  displayed logical pixels, so they match UI layouts and screenshots after the
  firmware changes orientation. Normalized coordinates from 0.0 to 1.0 are
  also accepted for existing scripts.
- `CROSSPOINT_SIM_SCREENSHOTS` saves BMP screenshots as
  `<milliseconds>:<path>`, separated by semicolons. Create the destination
  directory before running the simulator.
- `CROSSPOINT_SIM_FREE_HEAP` and `CROSSPOINT_SIM_MAX_ALLOC_HEAP` override the
  ESP heap metrics reported to firmware. They are useful for repeatable
  low-memory paths without exhausting the host process. Values are byte counts;
  invalid or out-of-range values use the 1 MiB default. The free-heap override
  also controls the reported minimum free heap, and maximum allocation is
  bounded by free heap.
- A sleep/wake test starts a fresh simulator process, matching the existing
  deep-sleep model. Set `CROSSPOINT_SIM_INPUT_SCRIPT_AFTER_WAKE` and
  `CROSSPOINT_SIM_SCREENSHOTS_AFTER_WAKE` for that second process. The
  pre-sleep schedules are cleared during relaunch so they cannot repeat
  forever.

Times are measured from process startup. For example:

```bash
mkdir -p ./qa-artifacts
CROSSPOINT_SIM_INPUT_SCRIPT='900:DOWN;1250:DOWN;1600:DOWN;1900:ENTER;3000:QUIT' \
CROSSPOINT_SIM_SCREENSHOTS='2400:./qa-artifacts/settings.bmp' \
  .pio/build/simulator/program
```

An X4 Pro touch and Home-key smoke test can use:

```bash
CROSSPOINT_SIM_INPUT_SCRIPT='2000:TAP:240,530;3000:HOME:100;3900:QUIT' \
CROSSPOINT_SIM_SCREENSHOTS='2500:./qa-artifacts/x4-pro-settings.bmp;3500:./qa-artifacts/x4-pro-home.bmp' \
  .pio/build/simulator_x4_pro/program
```

For Sticky, the same touch path is available without the Home key:

```bash
CROSSPOINT_SIM_INPUT_SCRIPT='2000:TAP:240,530;3600:QUIT' \
CROSSPOINT_SIM_SCREENSHOTS='1500:./qa-artifacts/sticky-home.bmp;3000:./qa-artifacts/sticky-settings.bmp' \
  .pio/build/simulator_sticky/program
```

A deterministic sleep/wake smoke test can use:

```bash
CROSSPOINT_SIM_INPUT_SCRIPT='900:SLEEP;3500:ENTER' \
CROSSPOINT_SIM_INPUT_SCRIPT_AFTER_WAKE='2200:QUIT' \
CROSSPOINT_SIM_SCREENSHOTS_AFTER_WAKE='1600:./qa-artifacts/wake.bmp' \
  .pio/build/simulator/program
```

The screenshot contains the SDL renderer output at the host's actual drawable
resolution, including Retina/HiDPI scaling. BMP is used because it is supported
directly by SDL2 and adds no image-encoding dependency to the simulator.

### Notes

**Host-backed network flows**: OPDS/catalog downloads and KOReader sync use the
host's `curl` binary through simulator implementations of `HTTPClient` and
`esp_http_client`. This keeps the firmware code path intact while allowing the
desktop build to reach real HTTP/HTTPS services.

**Mocked downloads**: Set `CROSSPOINT_SIM_HTTP_MOCK_ROOT` to a folder of local
fixtures to make host-backed HTTP requests return local files by basename before
falling back to the real network. This is useful for SD-font testing because the
firmware can request its normal release URLs while the simulator serves a local
`fonts.json` and `.cpfont` files:

```bash
cd /path/to/firmware
python3 -m pip install -r lib/EpdFont/scripts/requirements.txt
python3 lib/EpdFont/scripts/build-sd-fonts.py \
  --only NotoSansExtended \
  --manifest \
  --base-url "https://github.com/crosspoint-reader/crosspoint-fonts/releases/download/local/"
CROSSPOINT_SIM_HTTP_MOCK_ROOT="$PWD/lib/EpdFont/scripts/output" \
  pio run -e simulator -t run
```

The mock still uses the firmware's normal manifest parsing, file download,
write-to-SD, `.cpfont` validation, registry refresh, and font-selection flow.

**File transfer**: The simulator provides host-backed `WebServer`,
`WebSocketsServer`, and `NetworkClient` shims so firmware-owned file-transfer
routes can run on the host. Firmware web servers that bind port 80 are exposed
on `http://127.0.0.1:8080/`; WebSocket servers that bind port 81 are exposed on
`ws://127.0.0.1:8081/`. Set `CROSSPOINT_SIM_HTTP_PORT` to another unprivileged
port if that pair is occupied; the WebSocket endpoint uses the following port.
For example, `CROSSPOINT_SIM_HTTP_PORT=18080` exposes HTTP on 18080 and
WebSocket on 18081. This supports the browser file manager, WebSocket upload
progress, streamed downloads, and common WebDAV-style requests such as
`OPTIONS`, `PROPFIND`, `PUT`, `DELETE`, `MKCOL`, `MOVE`, and `COPY`. WebDAV
`LOCK` and `UNLOCK` remain compatibility-only unless the firmware implements
locking semantics.

The `run_simulator` target also accepts the port through PlatformIO, which is
convenient when the conflict is permanent on a development machine:

```ini
[env:simulator]
custom_simulator_http_port = 18080
```

Direct binary launches use the environment variable form.

**Firmware updates**: OTA and SD-card firmware flashing are non-destructive in
the simulator. The simulator stubs those update paths so the UI can be opened
without flashing firmware or changing boot partitions.

**Image previews**: The default simulator shims decode JPEG and PNG files on the
host and render a rough grayscale preview through the firmware's normal image
callbacks. This is meant to make image pages and PNG sleep overlays visible
while testing desktop flows. Native decoder libraries can be enabled with the
sample config's opt-in flags when decoder compatibility matters more than the
self-contained default. Neither mode simulates device-specific e-ink image
quality, refresh behaviour, or memory pressure.

**Cache**: On first open of an ebook, an "Indexing..." popup will appear while the section cache is built. If you see rendering issues after a code change that affects layout, delete `./fs_/.crosspoint/` to clear stale caches.

> [!WARNING]
> **Upstream compatibility:** The simulator mirrors interfaces used by Crosspoint. If Crosspoint adds or changes methods in a shared library and the simulator build reaches that code path, the simulator can fail to compile or link until a matching implementation or stub is added here. In many cases this is just a small no-op shim. Open a PR if the change tracks upstream CrossPoint, fills a gap in the emulated Arduino/ESP-IDF layer, or fixes the simulator itself. If the change only matches your own fork's HAL, maintain it in a fork of this repo instead. See [FORKING.md](FORKING.md).
