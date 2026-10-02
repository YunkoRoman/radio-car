# Radio car receiver

ESP8266 (NodeMCU v2) firmware for the car, built step by step with PlatformIO.

The current firmware is a board smoke test: the built-in LED (GPIO2) toggles
every 500 ms and the car MAC address is printed to the Serial Monitor (115200).
That MAC goes into `RECEIVER_MAC` in `controller/include/radio_config.h`.

```bash
pio run -t upload
pio device monitor -b 115200
```

For a Wemos D1 mini, set `board = d1_mini` in `platformio.ini`. After adding a
new library include, regenerate the clangd database with `pio run -t compiledb`.
