# Radio car controller

The project is set up for VS Code with PlatformIO, targeting Arduino Uno and ESP32.

## AI change log

Architectural decisions made by AI are recorded in `design-log/` as separate
numbered documents. The catalog [design-log/index.md](design-log/index.md)
lists them. The rules and the `/design-log` command are described in
`AGENTS.md` and `.claude/skills/design-log/SKILL.md`.

## ESP32-C3 remote

The remote is being built step by step. The current firmware reads the
joysticks of an ESP32-C3 SuperMini: throttle (left stick, Y axis) on GPIO0,
steering (right stick, X axis) on GPIO1, 12-bit ADC (0…4095). Values are
calibrated and normalized to `-1000…1000` (`include/joystick.h`): throttle
forward and steering right are positive, dead zone ±60. Every 100 ms the
normalized and raw values are printed to the Serial Monitor (115200).

```bash
pio run -e remote_esp32c3 -t upload
pio device monitor -b 115200
```

Normalization host test:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude test/test_joystick/test_joystick.cpp -o /tmp/test_joystick && /tmp/test_joystick
```

The ESP-NOW protocol design is described in `design-log/003-esp-now-remote-controller.md`.

## Getting started

1. Open this folder in VS Code.
2. Install the recommended **PlatformIO IDE** extension and restart VS Code.
3. Connect the board via USB.
4. In the PlatformIO panel run **Upload**, then **Serial Monitor**.

The default build target is the `remote_esp32c3` remote. For Arduino Uno,
select the `uno` environment in the VS Code status bar or run:

```bash
pio run -e uno -t upload
```

For ESP32:

```bash
pio run -e esp32dev -t upload
```

If the port is not detected automatically, add these lines to the relevant
environment in `platformio.ini` (use your own port):

```ini
upload_port = /dev/ttyUSB0
monitor_port = /dev/ttyUSB0
```

On Linux the user may need access to the serial port:

```bash
sudo usermod -aG dialout $USER
```

Then log out and log back in. For a different board model, change the `board`
value in `platformio.ini`; the identifier can be found with `pio boards`.
