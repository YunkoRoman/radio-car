#include <Arduino.h>
#include <ESP8266WiFi.h>

// Built-in LED on GPIO2, active LOW (NodeMCU and Wemos D1 mini).
constexpr uint32_t BLINK_INTERVAL_MS = 500;

void setup() {
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT);
  WiFi.mode(WIFI_STA);
}

void loop() {
  digitalWrite(LED_BUILTIN, !digitalRead(LED_BUILTIN));
  // MAC goes into RECEIVER_MAC in controller/include/radio_config.h.
  Serial.printf("car MAC %s\n", WiFi.macAddress().c_str());
  delay(BLINK_INTERVAL_MS);
}
