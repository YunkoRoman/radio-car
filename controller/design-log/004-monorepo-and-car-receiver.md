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

### 2026-10-02: ESP-NOW receiver and shared protocol header

- `ControlPacket` moved from `controller/include/` to `shared/control_packet.h`;
  both `platformio.ini` files add `-I../shared`. The remote binary size did not
  change.
- `car/src/main.cpp` is now an ESP-NOW receiver on channel 1 (role SLAVE,
  accepts any sender, so the remote's broadcast arrives). The receive callback
  rejects packets with the wrong size or protocol version (`bad`), counts
  sequence gaps as `lost` (gaps ≥1000 are treated as a remote restart), and
  stores the latest packet. `loop()` snapshots it every 200 ms and prints
  `NO SIGNAL` when no packet arrived for 300 ms — the hook for the future
  motor failsafe.
- First hardware test received only ~5% of packets (`lost` 56–60 between
  arrivals). Two changes together fixed it: remote TX power lowered to
  8.5 dBm (`WiFi.setTxPower`, known ESP32-C3 SuperMini antenna issue) and
  ESP8266 modem sleep disabled (`WIFI_NONE_SLEEP`). Both boards now print
  their actual channel. Retest: 491 packets in 10 s, `lost=0`, `bad=0`,
  packet age 13–17 ms. Which change mattered was not isolated.
- clangd: the pioarduino IDE extension writes `.cache/clangd/compile_commands.json`
  and injects the ESP32 core include path even for ESP8266, which broke
  `Arduino.h` resolution in the editor. `car/.clangd` removes that path; a root
  `.clangd` strips both toolchains' GCC-only flags and forces C++ for headers
  in `shared/`. All four clangd checks report 0 errors.
- Known nit: `loop()` reads `millis()` before the snapshot, so a packet landing
  in between could produce one spurious `NO SIGNAL` line; fix when the
  failsafe starts driving motors.

### 2026-10-02: link-state RGB LED

- The car board is a Witty Cloud (ESP-12F, CH340, RGB LED); `nodemcuv2`
  remains the build target since the module and flash size match.
- Added `LinkState { Searching, Connected, Lost }` in `car/src/main.cpp`,
  derived each `loop()` pass from the packet snapshot: no packet since boot →
  Searching (blue, 500 ms blink); last packet ≤300 ms old → Connected (steady
  green); otherwise Lost (red, 125 ms blink). RGB pins: red GPIO15, green
  GPIO12, blue GPIO13, HIGH = on. State changes print `link: …`; a boot
  self-test flashes red, green, blue.
- Fixed the earlier nit: `millis()` is now read inside the snapshot.
- Verified on hardware: user confirmed colours; serial showed
  `link: CONNECTED` with 1 lost packet out of 179.
