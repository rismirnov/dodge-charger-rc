#pragma once
#include "IMotor.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
class Drv8837Motor : public IMotor {
   public:
    struct Config {
        gpio_num_t in1;
        gpio_num_t in2;
        gpio_num_t sleep;
        ledc_channel_t in1Channel;
        ledc_channel_t in2Channel;
        uint32_t pwmFrequency;
    };
    explicit Drv8837Motor(const Config& config);
    void init() override;
    void forward(float power) override;
    void reverse(float power) override;
    void stop() override;

   private:
    void setDuty(ledc_channel_t channel, uint32_t duty);
    void setSpeed(float speed);
    Config config_;
};
