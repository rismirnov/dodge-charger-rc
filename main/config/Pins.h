#pragma once

#include "driver/gpio.h"
#include "driver/ledc.h"

namespace Pins {
    constexpr gpio_num_t SteeringServo = GPIO_NUM_18;
    constexpr ledc_channel_t SteeringChannel = LEDC_CHANNEL_0;

    constexpr gpio_num_t MotorIn1 = GPIO_NUM_25;
    constexpr gpio_num_t MotorIn2 = GPIO_NUM_26;
    constexpr gpio_num_t MotorSleep = GPIO_NUM_27;
    constexpr ledc_channel_t MotorIn1Channel = LEDC_CHANNEL_1;
    constexpr ledc_channel_t MotorIn2Channel = LEDC_CHANNEL_2;

    constexpr gpio_num_t Headlights = GPIO_NUM_32;
    constexpr gpio_num_t RearLights = GPIO_NUM_33;
}
