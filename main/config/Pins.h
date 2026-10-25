#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"
#include <driver/i2s_types.h>

namespace Pins {
    // ЛЕВАЯ СТОРОНА — Аудиофил-зона (I2S цифровой звук)
    constexpr gpio_num_t AudioBclk = GPIO_NUM_18;
    constexpr gpio_num_t AudioWs   = GPIO_NUM_19;
    constexpr gpio_num_t AudioDin  = GPIO_NUM_5;

    // ПРАВАЯ СТОРОНА — Силовая зона (Моторы, Сервопривод, Свет)
    // Драйвер мотора — DRV8837
    constexpr gpio_num_t MotorIn1    = GPIO_NUM_32; // (Остались в углу для удобства)
    constexpr gpio_num_t MotorIn2    = GPIO_NUM_33; 
    constexpr gpio_num_t MotorSleep  = GPIO_NUM_13;

    constexpr ledc_channel_t MotorIn1Channel = LEDC_CHANNEL_1;
    constexpr ledc_channel_t MotorIn2Channel = LEDC_CHANNEL_2;

    // Рулевой сервопривод — SG90 / ES08MA II
    constexpr gpio_num_t SteeringServo = GPIO_NUM_14;
    constexpr ledc_channel_t SteeringChannel = LEDC_CHANNEL_0;

    // Освещение — транзисторные ключи
    constexpr gpio_num_t Headlights = GPIO_NUM_26;
    constexpr gpio_num_t RearLights = GPIO_NUM_25;
    constexpr ledc_timer_t LightsTimer = LEDC_TIMER_2;
    constexpr ledc_channel_t HeadlightsChannel = LEDC_CHANNEL_3;
    constexpr ledc_channel_t RearLightsChannel = LEDC_CHANNEL_4;
}