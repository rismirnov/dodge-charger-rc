#pragma once
#include "ISteering.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
class Sg90Steering final : public ISteering {
   public:
    struct Config {
        gpio_num_t pin;
        ledc_channel_t channel;
        uint32_t frequency = 50;
        float minPulseUs = 1000.0f;
        float centerPulseUs = 1500.0f;
        float maxPulseUs = 2000.0f;
    };
    explicit Sg90Steering(const Config& config);
    void init();
    void setPosition(float position) override;
    void center() override;

   private:
    Config config_;
    void setPulse(float pulseUs);
};
