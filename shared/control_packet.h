#pragma once

#include <stdint.h>

// Bump when the wire layout changes; the receiver drops other versions.
constexpr uint8_t CONTROL_PROTOCOL_VERSION = 1;

// Shared by controller/ (sender) and car/ (receiver); both add -I../shared.
// Sent over ESP-NOW, little-endian on both ESP32 and ESP8266.
struct __attribute__((packed)) ControlPacket {
  uint8_t version;
  uint16_t sequence;  // wraps at 65535; lets the receiver spot lost packets
  int16_t throttle;   // -1000..1000, positive = forward
  int16_t steering;   // -1000..1000, positive = right
};

static_assert(sizeof(ControlPacket) == 7, "ControlPacket wire size changed");
