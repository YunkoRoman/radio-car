#pragma once

#include <stdint.h>

// Raw 12-bit ADC calibration for one stick axis.
struct AxisCalibration {
  int min;       // raw value treated as full deflection toward 0
  int center;    // raw value at rest
  int max;       // raw value treated as full deflection toward 4095
  int deadZone;  // raw counts around center that read as 0
  bool inverted; // true: low raw value means positive output
};

constexpr int16_t AXIS_LIMIT = 1000;

// Maps a raw reading to -1000..1000. Each half is scaled separately because
// the ESP32 ADC centre is not at mid-range.
inline int16_t normalizeAxis(int raw, const AxisCalibration& c) {
  if (raw < c.min) raw = c.min;
  if (raw > c.max) raw = c.max;

  int delta = raw - c.center;
  int32_t value = 0;
  if (delta > c.deadZone) {
    value = int32_t(delta - c.deadZone) * AXIS_LIMIT / (c.max - c.center - c.deadZone);
  } else if (delta < -c.deadZone) {
    value = int32_t(delta + c.deadZone) * AXIS_LIMIT / (c.center - c.min - c.deadZone);
  }
  return int16_t(c.inverted ? -value : value);
}
