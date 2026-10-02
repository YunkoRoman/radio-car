# Radio car controller

Проєкт налаштовано для роботи у VS Code через PlatformIO з Arduino Uno та ESP32.

## Журнал змін AI

Архітектурні рішення, ухвалені AI, фіксуються у `design-log/` як окремі
пронумеровані документи. Каталог [design-log/index.md](design-log/index.md)
містить їхній перелік. Правила та команда `/design-log` описані в `AGENTS.md`
і `.claude/skills/design-log/SKILL.md`.

## Пульт ESP32-C3

Пульт будується крок за кроком. Зараз прошивка читає сирі значення
джойстиків ESP32-C3 SuperMini: газ (лівий стік, вісь Y) на GPIO0, кермо
(правий стік, вісь X) на GPIO1, АЦП 12 біт (0…4095). Значення
калібруються й нормалізуються до `-1000…1000` (`include/joystick.h`):
газ уперед і кермо вправо — додатні, мертва зона ±60. Кожні 100 мс у Serial
Monitor (115200) друкуються нормалізовані та сирі значення.

Хост-тест нормалізації:

```bash
g++ -std=c++17 -Wall -Wextra -Werror -Iinclude test/test_joystick/test_joystick.cpp -o /tmp/test_joystick && /tmp/test_joystick
```

```bash
pio run -e remote_esp32c3 -t upload
pio device monitor -b 115200
```

Задум протоколу ESP-NOW описано в `design-log/003-esp-now-remote-controller.md`.

## Перший запуск

1. Відкрийте цю папку у VS Code.
2. Встановіть рекомендоване розширення **PlatformIO IDE** і перезапустіть VS Code.
3. Під'єднайте плату через USB.
4. На панелі PlatformIO виконайте **Upload**, потім **Serial Monitor**.

За замовчуванням збирається пульт `remote_esp32c3`. Для Arduino Uno виберіть
середовище `uno` на нижній панелі VS Code або виконайте:

```bash
pio run -e uno -t upload
```

Для ESP32:

```bash
pio run -e esp32dev -t upload
```

Якщо порт не визначився автоматично, додайте до потрібного середовища у
`platformio.ini` рядки (підставте свій порт):

```ini
upload_port = /dev/ttyUSB0
monitor_port = /dev/ttyUSB0
```

На Linux користувачеві може знадобитися доступ до serial-порту:

```bash
sudo usermod -aG dialout $USER
```

Після цього вийдіть із системи та увійдіть знову. Для іншої моделі плати
замініть значення `board` у `platformio.ini`; потрібний ідентифікатор можна
знайти командою `pio boards`.
