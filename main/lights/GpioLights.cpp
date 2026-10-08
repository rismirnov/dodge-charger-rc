#include "GpioLights.h"
GpioLights::GpioLights(const Config& config) : config_(config) {}
void GpioLights::init() {
    gpio_config_t config{};
    config.pin_bit_mask = (1ULL << config_.headlights) | (1ULL << config_.rearLights);
    config.mode = GPIO_MODE_OUTPUT;
    config.pull_up_en = GPIO_PULLUP_DISABLE;
    config.pull_down_en = GPIO_PULLDOWN_DISABLE;
    config.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&config);
    off();
}
void GpioLights::setHeadlights(bool enabled) { gpio_set_level(config_.headlights, enabled ? 1 : 0); }
void GpioLights::setRearLights(bool enabled) { gpio_set_level(config_.rearLights, enabled ? 1 : 0); }
void GpioLights::off() { setHeadlights(false); setRearLights(false); }
