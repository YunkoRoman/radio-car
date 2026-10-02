# Design Log Index

Catalog of architectural decisions. Add an entry here whenever a new design log
is created or its scope materially changes.

Status meanings:

- **drafted** — design written; scope or details may still change.
- **locked** — design frozen; implementation can begin.
- **completed** — implemented and verified; the entry contains implementation
  results.

| # | Title | Status | Description |
| --- | --- | --- | --- |
| 001 | [Arduino and ESP32 PlatformIO foundation](001-platformio-foundation.md) | completed | VS Code project foundation supporting a typical ESP32 development board and Arduino Uno through PlatformIO. |
| 002 | [Numbered architectural design-log workflow](002-design-log-workflow.md) | completed | Replaces the flat per-update journal with a numbered, indexed decision log and `/design-log` procedure. |
| 003 | [ESP-NOW remote controller protocol](003-esp-now-remote-controller.md) | locked | ESP32-C3 joystick transmitter sends normalized throttle and steering commands to an ESP8266 receiver every 20 ms. |
| 004 | [Single repository and ESP8266 car project](004-monorepo-and-car-receiver.md) | drafted | Repo root moves to `radio_car/`; new `car/` PlatformIO project for the ESP8266 receiver with a LED/MAC smoke test. |
