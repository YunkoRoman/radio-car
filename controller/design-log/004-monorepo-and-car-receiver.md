# 004 — Single repository and ESP8266 car project

## Background

The git repository lived inside `controller/` and held only the ESP32-C3
remote. The car receiver (ESP8266) from design 003 had no project yet; the
sibling `car/` folder was empty and outside version control.

## Problem

The remote and the car share the ESP-NOW `ControlPacket` protocol and must
evolve together, but they were split between a tracked and an untracked
folder.

## Design

- The repository root moves to `radio_car/`. The remote lives in
  `controller/`, the receiver in `car/`; each is an independent PlatformIO
  project with its own `platformio.ini`, `.gitignore`, `.clangd`, and README.
  A root README maps the folders.
- `car/` uses environment `car_esp8266`, board `nodemcuv2`, Arduino framework,
  115200 baud. The first firmware is a smoke test: built-in LED (GPIO2) toggles
  every 500 ms and the station MAC is printed, to be copied into
  `controller/include/radio_config.h`.
- `car/.clangd` strips xtensa-lx106 GCC-only flags (`-free`, `-fipa-pta`,
  `-mlongcalls`, `-mtext-section-literals`) that clangd rejects.

## Questions and Answers

**Q: One repository or a separate one for the car?**

**A:** One repository; the user chose it because both sides share the
protocol.

**Q: Which ESP8266 board?**

**A:** Not confirmed yet. `nodemcuv2` is the default; a Wemos D1 mini only
needs `board = d1_mini`.

## Trade-offs

- History is kept, but every remote file path gained a `controller/` prefix.
- `ControlPacket` is still defined only in `controller/include/`; the car will
  need a copy or a shared folder once it receives packets.
- The design log stays in `controller/design-log/` to avoid moving history.

## Implementation Results

- Moved `.git` from `controller/` to the root; git recorded the remote files as
  renames. Commit `a29e318` pushed to `origin/main`.
- `car/` builds with Espressif8266 Arduino: RAM 34.5%, flash 25.7%.
  `clangd --check=src/main.cpp` reports 0 errors.
- Not yet run on hardware: no ESP8266 connected.
