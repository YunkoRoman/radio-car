#pragma once

#include <stdint.h>

// Car receiver (ESP8266) station MAC. Unknown for now, so packets go to the
// broadcast address. Replace with the real MAC and set the flag to false.
constexpr uint8_t RECEIVER_MAC[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
constexpr bool RECEIVER_MAC_IS_PLACEHOLDER = true;

// Both boards must use the same fixed channel.
constexpr uint8_t ESPNOW_CHANNEL = 1;
constexpr uint32_t CONTROL_SEND_INTERVAL_MS = 20;
