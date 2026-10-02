#pragma once

#include <stdint.h>

// MG90S pulse calibration for direct-drive steering (no linkage). Keep the range
// small enough that the wheels never hit the chassis at full lock.
struct ServoCalibration {
  int centerUs;   // pulse that points the wheels straight
  int rangeUs;    // pulse offset at full lock (steering = ±1000)
  bool inverted;  // true if positive steering turns the wheels left
};

// Maps steering -1000..1000 (positive = right) to a servo pulse in µs.
inline int steeringToPulseUs(int16_t steering, const ServoCalibration& c) {
  if (steering > 1000) steering = 1000;
  if (steering < -1000) steering = -1000;
  int offset = int32_t(steering) * c.rangeUs / 1000;
  return c.centerUs + (c.inverted ? -offset : offset);
}
