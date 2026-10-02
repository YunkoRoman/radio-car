# 003 — ESP-NOW remote controller protocol

## Background

The project has PlatformIO support for a generic ESP32 and Arduino Uno, but no
firmware architecture for the ESP32-C3 handheld transmitter. The planned car
receiver is an ESP8266 controlling TT motors and steering.

## Problem

The transmitter needs low-latency joystick control without a Wi-Fi router, a
stable protocol the ESP8266 can consume later, and safe loss-of-signal behaviour
defined before receiver work starts.

## Design

The remote uses ESP-NOW on an ESP32-C3 Super Mini. Its two 3.3 V-powered
joysticks provide throttle on GPIO0 and steering on GPIO1. Firmware sends one
versioned `ControlPacket` every 20 ms containing a sequence number and two
signed normalized values (`-1000…1000`).

Receiver-specific values, including the placeholder peer MAC address, live in
`include/receiver_config.h`; the transmitter logic stays independent of the
receiver board. The eventual receiver must enter a safe state if no valid packet
arrives for 300 ms: motors stop and steering centres.

Both boards use fixed Wi-Fi channel 1. While the MAC is the all-zero placeholder,
the transmitter intentionally does not add it as an ESP-NOW peer; sends fail
without blocking startup. After adding the real station MAC, configuration flips
the placeholder flag and the peer is added on channel 1.

## Questions and Answers

**Q: Which radio transport should the first version use?**

**A:** ESP-NOW. It requires no router and suits short, low-latency control
packets between ESP32-C3 and ESP8266.

**Q: What does a centred joystick mean?**

**A:** Stop for throttle and straight for steering. A ±60 dead zone suppresses
minor ADC and mechanical noise.

**Q: How is the missing ESP8266 MAC handled now?**

**A:** A placeholder is isolated in configuration and replaced when the receiver
is available.

## Trade-offs

- One-way ESP-NOW keeps the first transmitter small and responsive, but provides
  no battery or receiver-state telemetry.
- Fixed calibration constants are simple but may need adjustment for individual
  joystick modules.
- GPIO0 and GPIO1 follow the requested wiring; board variants should be checked
  if boot-time hardware behaviour differs.

## Implementation Results

- Added `remote_esp32c3` (`esp32-c3-devkitm-1`) as the default PlatformIO
  environment, plus the `ControlPacket`, joystick normalization, and receiver
  configuration headers.
- Replaced the LED smoke test with the ESP-NOW transmitter. It samples GPIO0
  and GPIO1 every 20 ms, sends a 7-byte versioned packet, and prints MAC,
  joystick values, and send status at 115200 baud.
- Added a host-side test for joystick centre, endpoints, and inversion. The
  test passed with `g++ -std=c++17 -Wall -Wextra -Werror -Iinclude
  test/test_joystick_input/test_joystick_input.cpp -o /tmp/test_joystick_input
  && /tmp/test_joystick_input`.
- `./.venv-platformio/bin/pio run -e remote_esp32c3` passed with PlatformIO
  6.1.19 and Espressif32 7.0.1. Firmware uses 37,588 bytes RAM (11.5%) and
  719,928 bytes flash (54.9%).
- Upload and physical joystick tests were not run: ESP32-C3 board and final
  ESP8266 MAC address are not connected yet. `RECEIVER_MAC` remains the safe
  all-zero placeholder.
- Code review found that an all-zero placeholder is not a valid ESP-NOW peer
  address and that a shared channel must be explicit. The transmitter now skips
  peer registration while the placeholder flag is true and pins both future
  boards to channel 1.

### 2026-10-01: restart from LED smoke test

- At the user's request, all transmitter code (`src/main.cpp`, the
  `include/` headers, and `test/test_joystick_input`) was removed so the remote
  can be rebuilt in small, verified steps. The protocol design above remains the
  reference for later steps but is not implemented right now.
- `src/main.cpp` is now an LED smoke test for the ESP32-C3 SuperMini: blue LED
  on GPIO8 (active LOW) toggles every 500 ms and logs `LED on` / `LED off` at
  115200 baud.
- `remote_esp32c3` gained `ARDUINO_USB_MODE=1` and `ARDUINO_USB_CDC_ON_BOOT=1`
  so `Serial` goes through the native USB port.
- Uploaded to the board on `/dev/ttyACM0` (Espressif USB JTAG/serial); serial
  output confirmed alternating `LED on` / `LED off`.

### 2026-10-02: raw joystick read verified

- `src/main.cpp` replaced the LED smoke test with a raw ADC check: throttle
  (left stick Y) on GPIO0, steering (right stick X) on GPIO1, 12-bit
  `analogRead` (0…4095), printed as `throttle=NNNN steering=NNNN` every 100 ms.
- User confirmed on hardware that the firmware works and both sticks report
  values. Next step: record centre/endpoint raw values for calibration and
  normalization to `-1000…1000` with the ±60 dead zone.

### 2026-10-02: joystick calibration and normalization

- Measured raw ranges on hardware: throttle centre ~2300 (noise ±10),
  steering centre ~2275; both axes reach ~1…9 and 4095 at the stops.
  Forward and left read low, back and right read 4095.
- Added `include/joystick.h` with `AxisCalibration` and `normalizeAxis()`:
  clamp to `min…max`, ±60 dead zone, each half scaled separately to ±1000
  because the ADC centre is not mid-range, optional inversion.
- `src/main.cpp` uses `THROTTLE_CAL{50, 2300, 4045, 60, true}` and
  `STEERING_CAL{50, 2275, 4045, 60, false}`, so positive means throttle
  forward and steering right. Prints normalized and raw values every 100 ms.
- Host test `test/test_joystick` passed (`g++ -std=c++17 -Wall -Wextra -Werror
  -Iinclude test/test_joystick/test_joystick.cpp`). Build: RAM 4.3%, flash
  18.9%. User confirmed on hardware: centre 0, stops ±1000 in expected
  directions.
- Next step: ESP-NOW `ControlPacket` transmission every 20 ms.

### 2026-10-02: ESP-NOW transmitter

- Added `include/control_packet.h` (7-byte packed `ControlPacket`, protocol
  version 1) and `include/radio_config.h` (receiver MAC, placeholder flag,
  channel 1, 20 ms send interval).
- Design change: while the receiver MAC is unknown, the placeholder is the
  broadcast address `FF:FF:FF:FF:FF:FF` instead of all zeros, and the peer is
  registered. This lets the radio path be verified now, and the future ESP8266
  receiver can accept packets before its MAC is configured. Broadcast frames
  are not acknowledged, so send callbacks always report success until a real
  peer MAC is set.
- `src/main.cpp` samples both sticks and sends a `ControlPacket` every 20 ms;
  it logs remote MAC, channel, peer, sequence, values, immediate send result,
  and callback ok/fail counters every 200 ms.
- Build: RAM 11.5%, flash 54.6%, no warnings. Uploaded to the ESP32-C3
  (remote MAC `44:B1:76:17:BC:B4`); serial showed `send=ESP_OK` and
  `fail=0` across 300+ packets at rest with throttle/steering 0.

### 2026-10-02: unicast to the car and end-to-end stick test

- `RECEIVER_MAC` is now the car's station MAC `C8:C9:A3:0B:D5:1B` with
  `RECEIVER_MAC_IS_PLACEHOLDER = false`, so the car acknowledges packets and
  the remote's `fail` counter reflects real losses.
- End-to-end test, both serial ports read at once while the user moved the
  sticks: the car received throttle +1000 forward / −1000 back, steering
  −1000 left / +1000 right, and 0/0 at rest.
- Loss check: over ~3,900 packets the remote reported `fail=55` and the car's
  `lost` rose by exactly 55 (~1.4%). An earlier 8 s window with the remote
  lying still had 0 failures, so handling the SuperMini likely affects the
  link; well below the 300 ms (~15 packet) failsafe threshold, but range
  should be tested.
