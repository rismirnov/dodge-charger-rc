# Rastar RC — ESP32 + Arduino Core + Bluepad32

Проект пропорционального управления Rastar Dodge Charger R/T 1970 1/16.

## Платформа

Проект основан на официальном `esp-idf-arduino-bluepad32-template` 4.2.2:

- ESP-IDF 5.4.2
- Arduino Core 3.2.1
- Bluepad32
- BTstack
- PlatformIO + ESP-IDF

Официальный шаблон: https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template

## Архитектура

```text
DualShock 4
    ↓
Bluepad32 Arduino API
    ↓
Ds4Controller
    ↓ ControllerState
Car
 ├── ServoSteering
 ├── Drv8837Motor
 └── GpioLights
```

В отличие от raw ESP-IDF Bluepad32 API здесь нет собственного `my_platform.c`, `Bluepad32Bridge` и `ControllerInput`: их роль выполняет штатный `bluepad32_arduino` компонент.

## Управление

- Left Stick X → руль
- Right Stick Y вверх → вперёд
- Right Stick Y вниз → назад
- X → передние фары ON
- Y → передние фары OFF
- A → задние фонари ON
- B → задние фонари OFF

Стики нормализуются внутри `Ds4Controller`.

R2/L2 для движения не используются.

При отключении контроллера:

- мотор останавливается;
- руль центрируется;
- свет выключается.

## GPIO

| Назначение | GPIO |
|---|---:|
| Servo | 18 |
| DRV8837 IN1 | 25 |
| DRV8837 IN2 | 26 |
| DRV8837 nSLEEP | 27 |
| Headlights | 32 |
| Rear lights | 33 |

LEDC
│
├── TIMER_0
│    └── CHANNEL_0 → GPIO18 → Servo
│                    50 Hz
│
└── TIMER_1
     ├── CHANNEL_1 → GPIO25 → DRV8837 IN1
     └── CHANNEL_2 → GPIO26 → DRV8837 IN2
                         20 kHz

## Подготовка компонентов

Этот архив содержит наш application code и конфигурацию проекта. Тяжёлые сторонние исходники Bluepad32/BTstack/Arduino Core намеренно не дублируются.

Самый надёжный способ получить полный checkout — клонировать официальный template рекурсивно и заменить его `main/` на каталог `main/` из этого проекта:

```bash
git clone --recursive https://github.com/ricardoquesada/esp-idf-arduino-bluepad32-template.git template
```

После этого скопировать из template:

```text
components/arduino
components/bluepad32
components/bluepad32_arduino
components/btstack
```

в этот проект.

## Сборка

Требуется ESP-IDF 5.4.2 и PlatformIO.

```bash
pio run -e esp32dev
pio run -e esp32dev -t upload
pio device monitor -b 115200
```

## Важное замечание по DualShock 4

Bluepad32 предоставляет унифицированные `a()`, `b()`, `x()`, `y()` для игровых контроллеров. В текущей архитектуре именно эти унифицированные кнопки используются для света; если потребуется привязать управление к физическим символам конкретно DS4, это лучше проверить отдельно на реальном контроллере.
