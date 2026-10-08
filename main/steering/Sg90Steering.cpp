#include "Sg90Steering.h"
#include <algorithm>
namespace {
constexpr ledc_timer_t Timer = LEDC_TIMER_0;
constexpr ledc_mode_t Mode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t Resolution = LEDC_TIMER_14_BIT;
constexpr uint32_t MaxDuty = (1u << 14) - 1;
}  // namespace
Sg90Steering::Sg90Steering(const Config& config) : config_(config) {}
void Sg90Steering::init() {
    ledc_timer_config_t t{};
    t.speed_mode = Mode;
    t.timer_num = Timer;
    t.duty_resolution = Resolution;
    t.freq_hz = config_.frequency;
    t.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&t);
    ledc_channel_config_t c{};
    c.gpio_num = config_.pin;
    c.speed_mode = Mode;
    c.channel = config_.channel;
    c.intr_type = LEDC_INTR_DISABLE;
    c.timer_sel = Timer;
    c.duty = 0;
    c.hpoint = 0;
    ledc_channel_config(&c);
    center();
}
void Sg90Steering::setPulse(float pulseUs) {
    const float periodUs = 1'000'000.0f / static_cast<float>(config_.frequency);
    pulseUs = std::clamp(pulseUs, 0.0f, periodUs);
    const uint32_t duty = static_cast<uint32_t>((pulseUs / periodUs) * MaxDuty);
    ledc_set_duty(Mode, config_.channel, duty);
    ledc_update_duty(Mode, config_.channel);
}
void Sg90Steering::setPosition(float position) {
    position = std::clamp(position, -1.0f, 1.0f);
    const float pulseUs = position < 0
                              ? config_.centerPulseUs + position * (config_.centerPulseUs - config_.minPulseUs)
                              : config_.centerPulseUs + position * (config_.maxPulseUs - config_.centerPulseUs);
    setPulse(pulseUs);
}
void Sg90Steering::center() {
    setPulse(config_.centerPulseUs);
}
