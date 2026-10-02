# Radio car

ESP-NOW radio-controlled car, built step by step with PlatformIO.

| Folder | Board | Role |
| --- | --- | --- |
| [`controller/`](controller/README.md) | ESP32-C3 SuperMini | Handheld remote: reads two joysticks and sends `ControlPacket` over ESP-NOW. |
| [`car/`](car/README.md) | ESP8266 (Witty Cloud) | Car receiver: drives motors and steering. |

Wiring diagrams, pin map and shopping list: [`docs/schematics.html`](docs/schematics.html).

Each folder is a separate PlatformIO project; open it on its own in VS Code.
Architectural decisions are logged in [`controller/design-log/`](controller/design-log/index.md).
