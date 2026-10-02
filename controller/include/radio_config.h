#pragma once

#include <stdint.h>

// Car receiver (ESP8266 Witty Cloud) station MAC, printed by car/ at startup.
// Unicast lets the car acknowledge each packet, so send failures are real.
// For a car whose MAC is unknown, use FF:FF:FF:FF:FF:FF and set the flag.
constexpr uint8_t RECEIVER_MAC[6] = {0xC8, 0xC9, 0xA3, 0x0B, 0xD5, 0x1B};
constexpr bool RECEIVER_MAC_IS_PLACEHOLDER = false;

// Both boards must use the same fixed channel.
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint32_t CONTROL_SEND_INTERVAL_MS = 20;
