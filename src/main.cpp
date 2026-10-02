#include <Arduino.h>

#include "joystick.h"

// Each stick: potentiometer wiper on the GPIO, ends on 3V3 and GND.
constexpr uint8_t THROTTLE_PIN = 0;  // left stick, Y axis
constexpr uint8_t STEERING_PIN = 1;  // right stick, X axis
constexpr uint32_t PRINT_INTERVAL_MS = 100;

// Measured 2026-10-02: forward/left read ~6, back/right read 4095.
// Positive output: throttle forward, steering right.
constexpr AxisCalibration THROTTLE_CAL{50, 2300, 4045, 60, true};
constexpr AxisCalibration STEERING_CAL{50, 2275, 4045, 60, false};

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 0..4095
}

void loop() {
  int throttleRaw = analogRead(THROTTLE_PIN);
  int steeringRaw = analogRead(STEERING_PIN);
  int16_t throttle = normalizeAxis(throttleRaw, THROTTLE_CAL);
  int16_t steering = normalizeAxis(steeringRaw, STEERING_CAL);
  Serial.printf("throttle=%5d (raw %4d) steering=%5d (raw %4d)\n",
                throttle, throttleRaw, steering, steeringRaw);
  delay(PRINT_INTERVAL_MS);
}
