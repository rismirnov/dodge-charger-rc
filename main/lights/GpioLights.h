#pragma once
#include "ILights.h"
#include "driver/gpio.h"
class GpioLights final : public ILights {
public:
    struct Config { gpio_num_t headlights; gpio_num_t rearLights; };
    explicit GpioLights(const Config& config);
    void init();
    void setHeadlights(bool enabled) override;
    void setRearLights(bool enabled) override;
    void off() override;
private:
    Config config_;
};
