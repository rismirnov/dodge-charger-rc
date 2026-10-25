
#pragma once

#include <cstdint>
#include "ILights.h"
#include "config/Pins.h"

class GpioLights: public ILights {
public:
    struct Config {
        gpio_num_t headlights = Pins::Headlights;
        gpio_num_t rearLights = Pins::RearLights;
    };

    explicit GpioLights(const Config& config);

    void init();
    void update();

    void setHeadlights(bool enabled);
    void setRearLights(bool enabled);
    void off();

private:
    struct Light {
        float current = 0.0f;
        float target = 0.0f;
    };

    static constexpr uint32_t PwmFrequencyHz = 5000;
    static constexpr ledc_timer_bit_t PwmResolution =
        LEDC_TIMER_10_BIT;
    static constexpr uint32_t MaxDuty =
        (1U << 10U) - 1U;

    static constexpr uint32_t FadeDurationMs = 250;

    Config config_;
    Light headlights_;
    Light rearLights_;

    int64_t lastUpdateUs_ = 0;

    void setTarget(Light& light, bool enabled);

    void updateLight(
        Light& light,
        ledc_channel_t channel,
        float deltaSeconds
    );
};
