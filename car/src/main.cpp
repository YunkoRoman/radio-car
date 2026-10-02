#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <espnow.h>

#include "control_packet.h"

constexpr uint8_t ESPNOW_CHANNEL = 1;  // must match the remote
constexpr uint32_t SIGNAL_TIMEOUT_MS = 300;
constexpr uint32_t PRINT_INTERVAL_MS = 200;

// Written in the ESP-NOW receive callback, read in loop().
volatile bool hasPacket = false;
volatile uint32_t lastPacketMs = 0;
volatile uint32_t received = 0;
volatile uint32_t lost = 0;      // gaps in the sequence number
volatile uint32_t rejected = 0;  // wrong size or protocol version
ControlPacket latest{};

uint32_t lastPrintMs = 0;

void onReceive(uint8_t*, uint8_t* data, uint8_t len) {
  if (len != sizeof(ControlPacket) || data[0] != CONTROL_PROTOCOL_VERSION) {
    rejected = rejected + 1;
    return;
  }
  ControlPacket packet;
  memcpy(&packet, data, sizeof(packet));
  if (hasPacket) {
    uint16_t gap = packet.sequence - latest.sequence - 1;
    if (gap < 1000) lost = lost + gap;  // larger gaps mean the remote restarted
  }
  latest = packet;
  lastPacketMs = millis();
  received = received + 1;
  hasPacket = true;
}

void setup() {
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.setSleepMode(WIFI_NONE_SLEEP);  // keep the radio listening all the time
  wifi_set_channel(ESPNOW_CHANNEL);

  if (esp_now_init() != 0) {
    Serial.println("esp_now_init failed");
    return;
  }
  esp_now_set_self_role(ESP_NOW_ROLE_SLAVE);
  esp_now_register_recv_cb(onReceive);
  Serial.printf("\ncar MAC %s, channel %u (actual %u), waiting for remote\n",
                WiFi.macAddress().c_str(), ESPNOW_CHANNEL, wifi_get_channel());
}

void loop() {
  uint32_t now = millis();
  if (now - lastPrintMs < PRINT_INTERVAL_MS) return;
  lastPrintMs = now;

  noInterrupts();
  ControlPacket packet = latest;
  bool any = hasPacket;
  uint32_t age = now - lastPacketMs;
  uint32_t rx = received, lostCount = lost, bad = rejected;
  interrupts();

  if (!any || age > SIGNAL_TIMEOUT_MS) {
    // Failsafe: later this stops the motors and centres steering.
    Serial.printf("NO SIGNAL rx=%lu lost=%lu bad=%lu\n", (unsigned long)rx,
                  (unsigned long)lostCount, (unsigned long)bad);
    return;
  }
  Serial.printf("seq=%5u throttle=%5d steering=%5d age=%3lums rx=%lu lost=%lu bad=%lu\n",
                packet.sequence, packet.throttle, packet.steering,
                (unsigned long)age, (unsigned long)rx, (unsigned long)lostCount,
                (unsigned long)bad);
}
