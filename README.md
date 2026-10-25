# Rastar RC — ESP32 + Bluepad32

Пропорциональное управление моделью Rastar Dodge Charger R/T 1970 1/16 по Bluetooth (DualShock 4 и другие геймпады Bluepad32): руль, мотор, свет, звук двигателя через I2S.

## Платформа

- **ESP-IDF** (чистый C/C++, без Arduino Core)
- **Bluepad32** — `components/bluepad32`
- **BTstack** — `external/btstack`
- **PlatformIO** — [pioarduino/platform-espressif32](https://github.com/pioarduino/platform-espressif32) (см. `platformio.ini`)
- **LittleFS** — раздел `storage`, WAV из каталога `data/` прошиваются при сборке
- **esp_audio_simple_player** — воспроизведение WAV (зависимость IDF Component Manager)

Кастомная платформа Bluepad32 (`uni_platform`) и колбэки BT — в `main/main.cpp`. Вход с контроллера — `Ds4Controller` → `ControllerState`.

## Архитектура

```text
Gamepad (BT)
    ↓
Bluepad32 (uni_platform в main.cpp)
    ↓
Ds4Controller → ControllerState
    ↓
carTask (10 ms)
    ↓
Car
 ├── ServoSteering
 ├── Drv8837Motor   (плавный разгон / торможение)
 ├── GpioLights     (PWM, плавное включение)
 └── SoundManager → AudioPlayer (I2S → MAX98357A)
         ↑
    LittleFS (/littlefs) — engine_start.wav, car_movement.wav, car_brake.wav
```

## Управление

| Ввод | Действие |
|------|----------|
| Left Stick X | Руль |
| Right Stick Y вверх | Вперёд (мощность по отклонению) |
| Right Stick Y вниз | Назад |

Стики нормализуются в `Ds4Controller` (ось ±512 → −1…1). R2/L2 не используются.

**Свет:** при движении `Car` включает фары автоматически (вперёд — передние, назад — передние + задние). Кнопки X/Y/A/B в парсере выставляют флаги в `ControllerState`, но в `carTask` пока не применяются.

**Звук:** при старте один раз `engine_start.wav`; при газе/заднем ходе — цикл `car_movement.wav`; при отпускании стика — остановка движения. `car_brake.wav` есть в образе LittleFS, из логики `Car` пока не вызывается.

**Отключение контроллера:** мотор останавливается сразу, руль в центр, свет и звук движения выключаются.

## GPIO (ESP32-WROOM-32)

Одна сторона модуля — «аудио», другая — «силовая». Нумерация из `main/config/Pins.h`.

| GPIO | Назначение |
|-----:|------------|
| 5 | MAX98357A — DIN |
| 18 | MAX98357A — BCLK |
| 19 | MAX98357A — WS/LRC |
| 14 | SG90 / ES08MA II — PWM руля |
| 32 | DRV8837 — IN1 |
| 33 | DRV8837 — IN2 |
| 26 | Передние фары (ключ) |
| 25 | Задние фонари (ключ) |

DRV8837 **nSLEEP** на плате заведён на 3.3 V постоянно (GPIO для sleep не используется).

### LEDC

| Таймер | Каналы | Назначение |
|--------|--------|------------|
| TIMER_0 | CH0 | Руль, 50 Hz |
| TIMER_1 | CH1, CH2 | Мотор IN1/IN2, 25 kHz |
| TIMER_2 | CH3, CH4 | Фары, PWM |

## Структура проекта

```text
main/           — приложение (car, motor, steering, lights, audio, controller)
components/     — Bluepad32 и служебные компоненты
external/btstack/
data/           — WAV для LittleFS
partitions.csv  — factory 2M + storage 1M
```

## Сборка и прошивка

Нужны PlatformIO и ESP-IDF (подтягиваются платформой). При первой сборке Component Manager скачает `littlefs` и `esp_audio_simple_player` в `managed_components/`.

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

Альтернативная плата: `-e esp32doit-devkit-v1`.

Громкость по умолчанию задаётся в `app_main` (`player.setVolume(40)`).

## DualShock 4 и кнопки

Bluepad32 отдаёт унифицированные `BUTTON_A/B/X/Y`. Для света в README раньше фигурировали X/Y/A/B; если понадобится ручной свет с геймпада, достаточно в `carTask` читать `state.headlights` / `state.rearLights` и вызывать `Car::setHeadlights` / `setRearLights`.
