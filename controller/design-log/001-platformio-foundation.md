# 001 — Arduino and ESP32 PlatformIO foundation

## Background

The controller repository was empty and had no editor, build, upload, or serial
monitor configuration for its microcontroller firmware.

## Problem

The project needed one VS Code workflow for both an ESP32 development board and
an Arduino Uno without maintaining two unrelated setups.

## Design

`platformio.ini` defines PlatformIO environments based on the Arduino framework:

- `esp32dev` is the default environment for a typical ESP32 Dev Module.
- `uno` targets Arduino Uno.

`src/main.cpp` is a portable smoke-test program. It prints a startup message at
115200 baud and blinks `LED_BUILTIN`; it falls back to GPIO 2 when a board does
not define the LED macro. `.vscode/extensions.json` recommends PlatformIO IDE
and `.vscode/settings.json` maps `.ino` files to C++.

## Questions and Answers

**Q: Which VS Code workflow should support both boards?**

**A:** PlatformIO. It provides per-board build, upload, dependency, and serial
monitor support from one project configuration.

**Q: Should a serial port be committed?**

**A:** No. PlatformIO detects it automatically. The README documents optional
`upload_port` and `monitor_port` settings for systems where detection fails.

## Trade-offs

- `esp32dev` is a generic ESP32 target; a specific board may require a different
  `board` identifier later.
- The startup program is intentionally minimal and does not implement vehicle
  control behaviour.

## Implementation Results

- Added `platformio.ini`, `src/main.cpp`, VS Code recommendations and settings,
  `.gitignore`, and setup instructions in `README.md`.
- Installed PlatformIO IDE 3.3.4 and Microsoft C/C++ Tools 1.32.2 in VS Code.
- Confirmed the installed extensions were listed by VS Code. A firmware build
  was not run because PlatformIO initialization requires restarting VS Code.
