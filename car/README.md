# Radio car receiver

ESP8266 (Witty Cloud, ESP-12F; built as `nodemcuv2`) firmware for the car, built step by step with PlatformIO.

The current firmware receives `ControlPacket` (`shared/control_packet.h`)
from the remote over ESP-NOW on channel 1 and prints the values, packet age,
and received/lost/rejected counters every 200 ms to the Serial Monitor
(115200). If no packet arrives for 300 ms it prints `NO SIGNAL`; later this
will stop the motors. The RGB LED shows the link: slow blue blink while searching (no packet since
boot), steady green when connected, fast red blink when the link is lost.
At boot it flashes red, green, blue once as a pin check. The car MAC is
printed at startup; it goes into
`RECEIVER_MAC` in `controller/include/radio_config.h`.

```bash
pio run -t upload
pio device monitor -b 115200
```

For a Wemos D1 mini, set `board = d1_mini` in `platformio.ini`. After adding a
new library include, regenerate the clangd database with `pio run -t compiledb`.
