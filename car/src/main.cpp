#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Servo.h>
#include <espnow.h>

#include "control_packet.h"
#include "steering.h"

constexpr uint8_t ESPNOW_CHANNEL = 1;  // must match the remote
constexpr uint32_t SIGNAL_TIMEOUT_MS = 300;
constexpr uint32_t PRINT_INTERVAL_MS = 200;

// Witty Cloud RGB LED, HIGH = on.
constexpr uint8_t LED_RED = 15;
constexpr uint8_t LED_GREEN = 12;
constexpr uint8_t LED_BLUE = 13;
constexpr uint32_t SEARCH_BLINK_MS = 500;
constexpr uint32_t LOST_BLINK_MS = 125;

enum class LinkState { Searching, Connected, Lost };

// MG90S steering servo, signal on GPIO4 (shared with the Witty button, which
// only pulls it low while pressed). Start small; widen after checking lock.
constexpr uint8_t SERVO_PIN = 4;
constexpr ServoCalibration SERVO_CAL{1150, 500, false};

// Written in the ESP-NOW receive callback, read in loop().
volatile bool hasPacket = false;
volatile uint32_t lastPacketMs = 0;
volatile uint32_t received = 0;
volatile uint32_t lost = 0;      // gaps in the sequence number
volatile uint32_t rejected = 0;  // wrong size or protocol version
ControlPacket latest{};

uint32_t lastPrintMs = 0;
LinkState shownState = LinkState::Searching;
Servo steeringServo;
int servoPulseUs = SERVO_CAL.centerUs;

void setLed(bool red, bool green, bool blue) {
  digitalWrite(LED_RED, red);
  digitalWrite(LED_GREEN, green);
  digitalWrite(LED_BLUE, blue);
}

// Shows each colour once so wrong pin mapping is obvious at boot.
void ledSelfTest() {
  const char* names[] = {"red", "green", "blue"};
  for (int i = 0; i < 3; i++) {
    Serial.printf("LED test: %s\n", names[i]);
    setLed(i == 0, i == 1, i == 2);
    delay(400);
  }
  setLed(false, false, false);
}

// Searching: slow blue blink. Connected: steady green. Lost: fast red blink.
void showLinkState(LinkState state, uint32_t now) {
  switch (state) {
    case LinkState::Searching:
      setLed(false, false, (now / SEARCH_BLINK_MS) % 2 == 0);
      break;
    case LinkState::Connected:
      setLed(false, true, false);
      break;
    case LinkState::Lost:
      setLed((now / LOST_BLINK_MS) % 2 == 0, false, false);
      break;
  }
}

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
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  Serial.println();
  ledSelfTest();
  // Wide attach limits; steeringToPulseUs keeps the pulse inside SERVO_CAL.
  steeringServo.attach(SERVO_PIN, 500, 2500, SERVO_CAL.centerUs);

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
  Serial.printf("car MAC %s, channel %u (actual %u), waiting for remote\n",
                WiFi.macAddress().c_str(), ESPNOW_CHANNEL, wifi_get_channel());
}

void loop() {
  noInterrupts();
  uint32_t now = millis();  // read with the snapshot so age never underflows
  ControlPacket packet = latest;
  bool any = hasPacket;
  uint32_t age = now - lastPacketMs;
  uint32_t rx = received, lostCount = lost, bad = rejected;
  interrupts();

  LinkState state = !any                       ? LinkState::Searching
                    : age > SIGNAL_TIMEOUT_MS  ? LinkState::Lost
                                               : LinkState::Connected;
  showLinkState(state, now);

  // Failsafe: without a fresh packet the wheels point straight.
  int pulse = state == LinkState::Connected ? steeringToPulseUs(packet.steering, SERVO_CAL)
                                            : SERVO_CAL.centerUs;
  if (pulse != servoPulseUs) {
    steeringServo.writeMicroseconds(pulse);
    servoPulseUs = pulse;
  }
  if (state != shownState) {
    const char* names[] = {"SEARCHING", "CONNECTED", "LOST"};
    Serial.printf("link: %s\n", names[static_cast<int>(state)]);
    shownState = state;
  }

  if (now - lastPrintMs < PRINT_INTERVAL_MS) return;
  lastPrintMs = now;

  if (state != LinkState::Connected) {
    // Failsafe: later this stops the motors and centres steering.
    Serial.printf("NO SIGNAL rx=%lu lost=%lu bad=%lu\n", (unsigned long)rx,
                  (unsigned long)lostCount, (unsigned long)bad);
    return;
  }
  Serial.printf("seq=%5u throttle=%5d steering=%5d servo=%4dus age=%3lums rx=%lu lost=%lu bad=%lu\n",
                packet.sequence, packet.throttle, packet.steering, servoPulseUs,
                (unsigned long)age, (unsigned long)rx, (unsigned long)lostCount,
                (unsigned long)bad);
}
