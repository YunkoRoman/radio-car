# ESP-NOW remote controller design

## Scope

This is the first implementation phase of the radio-controlled car project:
the handheld transmitter only. It runs on an ESP32-C3 Super Mini and sends
throttle and steering commands to a future ESP8266 receiver.

## Hardware

- ESP32-C3 Super Mini powered from the 5 V output of the MH-CD42 module.
- Two joystick modules powered from the board's 3.3 V pin.
- Throttle joystick `VRy` connects to GPIO0.
- Steering joystick `VRx` connects to GPIO1.
- All modules share ground.

The joystick supply must remain at 3.3 V so their analogue outputs never
exceed the ESP32-C3 ADC input range.

## Firmware architecture

PlatformIO gains a `remote_esp32c3` environment using
`esp32-c3-devkitm-1` and the Arduino framework. The sender has no external
libraries: ESP-NOW is supplied by the ESP32 Arduino core.

`src/main.cpp` owns three responsibilities:

1. Configure the ADC pins and Wi-Fi station mode.
2. Read, centre, dead-zone, and normalize both joystick values.
3. Send a control packet every 20 ms through ESP-NOW and print diagnostic
   status to Serial Monitor at 115200 baud.

`include/receiver_config.h` isolates receiver-specific configuration. It holds
the placeholder ESP8266 MAC address, its placeholder flag, and tunable
constants for joystick centre, dead zone, packet interval, and Wi-Fi channel.
Replacing the receiver later requires editing only that file.

## ESP-NOW protocol

The transmitter sends a fixed-size versioned payload every 20 ms (50 Hz):

```cpp
struct ControlPacket {
  uint8_t version;
  uint16_t sequence;
  int16_t throttle;
  int16_t steering;
};
```

`throttle` and `steering` are normalized integer values in the inclusive range
`-1000…1000`:

- throttle positive: forward; negative: reverse;
- steering positive: right; negative: left;
- both zero: stopped and straight.

The sequence number increments for each send and helps receiver diagnostics.
Both boards use fixed Wi-Fi channel 1. The first receiver firmware will require
a packet at least every 300 ms; on timeout it must stop the drive motors and
centre the steering servo.

## Input processing

Each raw ADC axis is mapped around its configured centre. A normalized dead
zone of ±60 removes joystick noise at rest. Values outside it are rescaled so
full travel still reaches -1000 or 1000. The initial version uses linear
response and no automatic calibration; constants can be updated after measuring
the actual joystick range.

## Diagnostics and validation

At startup the transmitter prints its own MAC address and the configured peer
MAC. During operation Serial Monitor reports the two command values, sequence
number, and ESP-NOW send callback result.

Validation for this phase:

1. Build PlatformIO environment `remote_esp32c3` successfully.
2. Confirm Serial output at 115200 baud and inspect joystick centre/full-range
   values.
3. With the temporary peer MAC, confirm send failures are observable.
4. Replace the MAC when the ESP8266 is built; confirm send callbacks succeed.

## Non-goals

- Receiver, motor, and servo firmware.
- Bidirectional telemetry, battery reporting, pairing UI, and automatic
  joystick calibration.
- Any Wi-Fi access point or router dependency.
