# ESP-NOW Remote Controller Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use
> `superpowers:subagent-driven-development` (recommended) or
> `superpowers:executing-plans` to implement this plan task-by-task. Steps use
> checkbox (`- [ ]`) syntax for tracking.

**Goal:** Build ESP32-C3 firmware that converts two joystick axes into ESP-NOW
control packets for the future ESP8266 car receiver.

**Architecture:** Keep protocol, joystick math, and receiver-specific settings
separate. `src/main.cpp` coordinates ESP-NOW and hardware I/O; pure headers
define the packet and normalization logic so it can be unit-tested without the
remote hardware.

**Tech Stack:** PlatformIO, Arduino framework for ESP32-C3, built-in ESP-NOW,
Wi-Fi station mode, host-side C++ assertion tests.

## Global Constraints

- Build board: `esp32-c3-devkitm-1`.
- Joysticks use 3.3 V only; never feed 5 V into GPIO0 or GPIO1.
- GPIO0 is throttle (`VRy`); GPIO1 is steering (`VRx`).
- Send a versioned packet every 20 ms, with normalized axes in `-1000…1000`.
- Use an all-zero placeholder ESP8266 MAC until the receiver exists; do not add
  it as an ESP-NOW peer.
- Pin both boards to Wi-Fi channel `1`.
- No external libraries and no Wi-Fi access point or router.
- The receiver will use a 300 ms command timeout; transmitter must send often
  enough for that watchdog.

---

## File structure

| File | Responsibility |
| --- | --- |
| `platformio.ini` | Adds the ESP32-C3 transmitter build and test environment. |
| `include/control_packet.h` | Defines the stable, packed ESP-NOW packet. |
| `include/joystick_input.h` | Defines pure axis normalization logic. |
| `include/receiver_config.h` | Holds peer MAC and calibration constants. |
| `src/main.cpp` | Initializes Wi-Fi/ESP-NOW, samples hardware, sends packets, emits diagnostics. |
| `test/test_joystick_input/test_joystick_input.cpp` | Tests normalization boundaries and dead zone. |
| `README.md` | Documents the remote environment, MAC replacement, and wiring. |

### Task 1: Define and test joystick normalization

**Files:**

- Create: `include/joystick_input.h`
- Create: `test/test_joystick_input/test_joystick_input.cpp`

**Interfaces:**

- Produces `struct AxisCalibration` and
  `int16_t normalizeAxis(int rawValue, const AxisCalibration& calibration)`.
- `AxisCalibration` fields: `minimum`, `center`, `maximum`, `deadZone`,
  `inverted`.
- Consumers: `src/main.cpp` in Task 3, configuration constants in Task 2.

- [ ] **Step 1: Write the failing normalization tests**

```cpp
#include <unity.h>
#include "joystick_input.h"

const AxisCalibration axis{0, 2048, 4095, 60, false};

void test_center_is_zero() {
  TEST_ASSERT_EQUAL_INT16(0, normalizeAxis(2048, axis));
  TEST_ASSERT_EQUAL_INT16(0, normalizeAxis(2108, axis));
  TEST_ASSERT_EQUAL_INT16(0, normalizeAxis(1988, axis));
}

void test_endpoints_reach_full_scale() {
  TEST_ASSERT_EQUAL_INT16(-1000, normalizeAxis(0, axis));
  TEST_ASSERT_EQUAL_INT16(1000, normalizeAxis(4095, axis));
}

void test_inversion_reverses_direction() {
  const AxisCalibration inverted{0, 2048, 4095, 60, true};
  TEST_ASSERT_EQUAL_INT16(1000, normalizeAxis(0, inverted));
  TEST_ASSERT_EQUAL_INT16(-1000, normalizeAxis(4095, inverted));
}

void setup() {
  UNITY_BEGIN();
  RUN_TEST(test_center_is_zero);
  RUN_TEST(test_endpoints_reach_full_scale);
  RUN_TEST(test_inversion_reverses_direction);
  UNITY_END();
}

void loop() {}
```

- [ ] **Step 2: Run the tests to prove they fail**

Run: `g++ -std=c++17 -Wall -Wextra -Werror -Iinclude test/test_joystick_input/test_joystick_input.cpp -o /tmp/test_joystick_input`

Expected: compilation fails because `joystick_input.h` does not exist.

- [ ] **Step 3: Implement minimal pure normalization logic**

```cpp
#pragma once

#include <stdint.h>

struct AxisCalibration {
  int minimum;
  int center;
  int maximum;
  int deadZone;
  bool inverted;
};

inline int16_t normalizeAxis(int rawValue, const AxisCalibration& axis) {
  const int offset = rawValue - axis.center;
  if (offset >= -axis.deadZone && offset <= axis.deadZone) return 0;

  int32_t value = 0;
  if (offset > 0) {
    value = static_cast<int32_t>(offset - axis.deadZone) * 1000 /
            (axis.maximum - axis.center - axis.deadZone);
  } else {
    value = static_cast<int32_t>(offset + axis.deadZone) * 1000 /
            (axis.center - axis.minimum - axis.deadZone);
  }
  if (value > 1000) value = 1000;
  if (value < -1000) value = -1000;
  return axis.inverted ? static_cast<int16_t>(-value)
                       : static_cast<int16_t>(value);
}
```

- [ ] **Step 4: Run the tests to prove they pass**

Run: `/tmp/test_joystick_input`

Expected: all 3 assertions pass without output. The test is host-side because
the normalization code has no Arduino dependency.

- [ ] **Step 5: Commit**

```bash
git add include/joystick_input.h test/test_joystick_input/test_joystick_input.cpp
git commit -m "feat: add joystick normalization"
```

### Task 2: Add transmitter configuration and protocol contract

**Files:**

- Modify: `platformio.ini`
- Create: `include/control_packet.h`
- Create: `include/receiver_config.h`

**Interfaces:**

- Produces `ControlPacket` with `version`, `sequence`, `throttle`, and
  `steering`; encoded size is exactly 7 bytes.
- Produces `RECEIVER_MAC`, `RECEIVER_MAC_IS_PLACEHOLDER`, `ESPNOW_CHANNEL`,
  `THROTTLE_AXIS`, `STEERING_AXIS`, and `CONTROL_SEND_INTERVAL_MS` for
  `src/main.cpp`.
- Consumes `AxisCalibration` from `joystick_input.h`.

- [ ] **Step 1: Add the ESP32-C3 environment**

Append this environment to `platformio.ini`:

```ini
[env:remote_esp32c3]
platform = espressif32
board = esp32-c3-devkitm-1
framework = arduino
monitor_speed = 115200
```

- [ ] **Step 2: Define the packet with an explicit wire layout**

Create `include/control_packet.h`:

```cpp
#pragma once

#include <stdint.h>

constexpr uint8_t CONTROL_PROTOCOL_VERSION = 1;

struct __attribute__((packed)) ControlPacket {
  uint8_t version;
  uint16_t sequence;
  int16_t throttle;
  int16_t steering;
};

static_assert(sizeof(ControlPacket) == 7, "ControlPacket wire size changed");
```

- [ ] **Step 3: Add one replaceable configuration file**

Create `include/receiver_config.h`:

```cpp
#pragma once

#include <stdint.h>
#include "joystick_input.h"

constexpr uint8_t RECEIVER_MAC[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
constexpr bool RECEIVER_MAC_IS_PLACEHOLDER = true;
constexpr uint8_t THROTTLE_PIN = 0;
constexpr uint8_t STEERING_PIN = 1;
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint32_t CONTROL_SEND_INTERVAL_MS = 20;

constexpr AxisCalibration THROTTLE_AXIS{0, 2048, 4095, 60, false};
constexpr AxisCalibration STEERING_AXIS{0, 2048, 4095, 60, false};
```

Add a comment immediately above `RECEIVER_MAC` saying it is a temporary value
that must be replaced with the ESP8266 station MAC before vehicle testing. When
replacing it, set `RECEIVER_MAC_IS_PLACEHOLDER` to `false`.

- [ ] **Step 4: Build the configured environment**

Run: `pio run -e remote_esp32c3`

Expected: PlatformIO resolves the ESP32-C3 platform and builds the existing
smoke-test firmware. If it fails because PlatformIO is not initialized after
the extension installation, restart VS Code once and rerun this command.

- [ ] **Step 5: Commit**

```bash
git add platformio.ini include/control_packet.h include/receiver_config.h
git commit -m "feat: configure ESP-NOW remote protocol"
```

### Task 3: Implement ESP-NOW transmitter firmware

**Files:**

- Modify: `src/main.cpp`
- Modify: `README.md`

**Interfaces:**

- Consumes `ControlPacket`, `normalizeAxis`, and all constants from Tasks 1–2.
- Produces ESP-NOW packets at 50 Hz and Serial diagnostics at 115200 baud.
- Uses ESP-NOW send callback only for diagnostics; a failed send must not stop
  later sends.

- [ ] **Step 1: Replace the LED smoke test with transmitter initialization**

Include `Arduino.h`, `WiFi.h`, `esp_now.h`, `control_packet.h`,
`joystick_input.h`, and `receiver_config.h`. In `setup()`:

```cpp
Serial.begin(115200);
pinMode(THROTTLE_PIN, INPUT);
pinMode(STEERING_PIN, INPUT);
WiFi.mode(WIFI_STA);
esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

if (esp_now_init() != ESP_OK) {
  Serial.println("ESP-NOW initialization failed");
  while (true) delay(1000);
}

esp_now_peer_info_t peerInfo{};
memcpy(peerInfo.peer_addr, RECEIVER_MAC, sizeof(RECEIVER_MAC));
peerInfo.channel = ESPNOW_CHANNEL;
peerInfo.encrypt = false;
if (esp_now_add_peer(&peerInfo) != ESP_OK) {
  Serial.println("ESP-NOW peer setup failed");
  while (true) delay(1000);
}
```

- [ ] **Step 2: Add diagnostics and packet sending**

Add a send callback that stores the latest `esp_now_send_status_t` in a
volatile variable. In `loop()`, use `millis()` to send only when at least
`CONTROL_SEND_INTERVAL_MS` elapsed:

```cpp
const ControlPacket packet{
    CONTROL_PROTOCOL_VERSION,
    sequence++,
    normalizeAxis(analogRead(THROTTLE_PIN), THROTTLE_AXIS),
    normalizeAxis(analogRead(STEERING_PIN), STEERING_AXIS),
};

const esp_err_t result = esp_now_send(
    RECEIVER_MAC, reinterpret_cast<const uint8_t*>(&packet), sizeof(packet));
```

Print the transmitter MAC, peer MAC, values, sequence number, immediate send
result, latest callback result, and Wi-Fi channel. While
`RECEIVER_MAC_IS_PLACEHOLDER` is true, skip `esp_now_add_peer()` so the all-zero
placeholder cannot block startup. Print each state once per second to keep
Serial Monitor readable; packet transmission stays at 50 Hz.

- [ ] **Step 3: Build firmware for the correct board**

Run: `pio run -e remote_esp32c3`

Expected: exit code 0. Resolve API differences using the ESP32 Arduino core
version installed by PlatformIO; do not change the packet layout.

- [ ] **Step 4: Upload and inspect hardware input values**

Run: `pio run -e remote_esp32c3 -t upload && pio device monitor -b 115200`

Expected: startup shows the transmitter MAC and placeholder peer MAC. At rest,
both normalized axes report 0; moving each stick through full travel reaches
approximately -1000 and 1000. If direction is reversed, change only the
matching `inverted` boolean in `receiver_config.h`.

- [ ] **Step 5: Document actual setup and replaceable values**

Add a `## Пульт ESP32-C3` section to `README.md` with:

- build/upload commands for `remote_esp32c3`;
- 5 V board supply and 3.3 V joystick supply warning;
- GPIO0 throttle and GPIO1 steering wiring;
- peer-MAC replacement instructions;
- 20 ms send interval and 300 ms receiver failsafe contract.

- [ ] **Step 6: Commit**

```bash
git add src/main.cpp README.md
git commit -m "feat: add ESP-NOW remote transmitter"
```

### Task 4: Verify end-to-end transmitter readiness

**Files:**

- Modify: `design-log/003-esp-now-remote-controller.md`

**Interfaces:**

- Consumes the built transmitter and the manual validation results from Task 3.
- Produces documented implementation results without changing the locked
  protocol decision.

- [ ] **Step 1: Run the complete automated verification set**

Run:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude test/test_joystick_input/test_joystick_input.cpp -o /tmp/test_joystick_input
/tmp/test_joystick_input
pio run -e remote_esp32c3
```

Expected: all host-side assertions and the transmitter build succeed.

- [ ] **Step 2: Perform manual transmitter checks**

Verify in Serial Monitor at 115200 baud:

1. Both centred sticks report `0`.
2. Throttle travel reaches negative and positive values near `1000`.
3. Steering travel reaches negative and positive values near `1000`.
4. The sender emits one packet roughly every 20 ms.
5. With the placeholder MAC, send failure is visible and firmware continues to
   sample/send without resetting.

- [ ] **Step 3: Record the factual result**

Append results to **Implementation Results** in
`design-log/003-esp-now-remote-controller.md`: exact build/test commands,
pass/fail result, measured inversion changes, and whether the real ESP8266 MAC
was available. Do not create a new design-log entry because the design does not
change.

- [ ] **Step 4: Commit**

```bash
git add design-log/003-esp-now-remote-controller.md
git commit -m "docs: record remote transmitter verification"
```

## Plan self-review

### Spec coverage

- ESP32-C3 and the stated 5 V/3.3 V wiring: Task 3 documentation and manual
  check.
- GPIO0 throttle and GPIO1 steering: Tasks 2–3.
- ESP-NOW without router: Task 3 Wi-Fi station and ESP-NOW setup.
- Versioned sequence packet and `-1000…1000` commands: Task 2 protocol and
  Task 1 normalization.
- 20 ms sender cadence and 300 ms receiver contract: Tasks 2–3 and README.
- Placeholder peer MAC and diagnostics: Tasks 2–4.
- Build, input, and send-result validation: Tasks 1, 3, and 4.

### Consistency checks

`ControlPacket`, `AxisCalibration`, `normalizeAxis`, `RECEIVER_MAC`, and
`remote_esp32c3` use the same names in every task. No external library is
introduced, and the receiver remains out of scope.
