#include <Arduino.h>
#include <WiFi.h>
#include <esp_now.h>
#include <esp_wifi.h>

#include "control_packet.h"
#include "joystick.h"
#include "radio_config.h"

// Each stick: potentiometer wiper on the GPIO, ends on 3V3 and GND.
constexpr uint8_t THROTTLE_PIN = 0;  // left stick, Y axis
constexpr uint8_t STEERING_PIN = 1;  // right stick, X axis
constexpr uint32_t PRINT_INTERVAL_MS = 200;

// Measured 2026-10-02: forward/left read ~6, back/right read 4095.
// Positive output: throttle forward, steering right.
constexpr AxisCalibration THROTTLE_CAL{50, 2300, 4045, 60, true};
constexpr AxisCalibration STEERING_CAL{50, 2275, 4045, 60, false};

// Updated from the Wi-Fi task in the send callback.
volatile uint32_t sendOk = 0;
volatile uint32_t sendFail = 0;

bool radioReady = false;
uint16_t sequence = 0;
uint32_t lastSendMs = 0;
uint32_t lastPrintMs = 0;

void onSent(const uint8_t*, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    sendOk = sendOk + 1;
  } else {
    sendFail = sendFail + 1;
  }
}

bool initRadio() {
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_channel(ESPNOW_CHANNEL, WIFI_SECOND_CHAN_NONE);

  if (esp_now_init() != ESP_OK) {
    Serial.println("esp_now_init failed");
    return false;
  }
  esp_now_register_send_cb(onSent);

  esp_now_peer_info_t peer{};
  memcpy(peer.peer_addr, RECEIVER_MAC, sizeof(RECEIVER_MAC));
  peer.channel = ESPNOW_CHANNEL;
  peer.ifidx = WIFI_IF_STA;
  peer.encrypt = false;
  if (esp_now_add_peer(&peer) != ESP_OK) {
    Serial.println("esp_now_add_peer failed");
    return false;
  }
  return true;
}

void setup() {
  Serial.begin(115200);
  analogReadResolution(12);  // 0..4095

  radioReady = initRadio();
  Serial.printf("remote MAC %s, channel %u, peer %02X:%02X:%02X:%02X:%02X:%02X%s\n",
                WiFi.macAddress().c_str(), ESPNOW_CHANNEL,
                RECEIVER_MAC[0], RECEIVER_MAC[1], RECEIVER_MAC[2],
                RECEIVER_MAC[3], RECEIVER_MAC[4], RECEIVER_MAC[5],
                RECEIVER_MAC_IS_PLACEHOLDER ? " (broadcast placeholder)" : "");
}

void loop() {
  uint32_t now = millis();
  if (now - lastSendMs < CONTROL_SEND_INTERVAL_MS) return;
  lastSendMs = now;

  int throttleRaw = analogRead(THROTTLE_PIN);
  int steeringRaw = analogRead(STEERING_PIN);
  ControlPacket packet{
      CONTROL_PROTOCOL_VERSION,
      sequence++,
      normalizeAxis(throttleRaw, THROTTLE_CAL),
      normalizeAxis(steeringRaw, STEERING_CAL),
  };

  esp_err_t result = ESP_FAIL;
  if (radioReady) {
    result = esp_now_send(RECEIVER_MAC, reinterpret_cast<const uint8_t*>(&packet),
                          sizeof(packet));
  }

  if (now - lastPrintMs >= PRINT_INTERVAL_MS) {
    lastPrintMs = now;
    Serial.printf("seq=%5u throttle=%5d steering=%5d send=%s ok=%lu fail=%lu\n",
                  packet.sequence, packet.throttle, packet.steering,
                  esp_err_to_name(result), (unsigned long)sendOk,
                  (unsigned long)sendFail);
  }
}
