#include "Drv8837Motor.h"
#include <algorithm>
#include <cmath>
namespace {
constexpr ledc_timer_t Timer = LEDC_TIMER_1;
constexpr ledc_mode_t Mode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t Resolution = LEDC_TIMER_10_BIT;
constexpr uint32_t MaxDuty = (1u << 10) - 1;
}  // namespace
Drv8837Motor::Drv8837Motor(const Config& config) : config_(config) {}
void Drv8837Motor::init() {
    gpio_config_t s{};
    s.pin_bit_mask = 1ULL << config_.sleep;
    s.mode = GPIO_MODE_OUTPUT;
    s.pull_up_en = GPIO_PULLUP_DISABLE;
    s.pull_down_en = GPIO_PULLDOWN_DISABLE;
    s.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&s);
    gpio_set_level(config_.sleep, 0);
    ledc_timer_config_t t{};
    t.speed_mode = Mode;
    t.timer_num = Timer;
    t.duty_resolution = Resolution;
    t.freq_hz = config_.pwmFrequency;
    t.clk_cfg = LEDC_AUTO_CLK;
    ledc_timer_config(&t);
    ledc_channel_config_t c1{};
    c1.gpio_num = config_.in1;
    c1.speed_mode = Mode;
    c1.channel = config_.in1Channel;
    c1.intr_type = LEDC_INTR_DISABLE;
    c1.timer_sel = Timer;
    c1.duty = 0;
    c1.hpoint = 0;
    ledc_channel_config(&c1);
    ledc_channel_config_t c2{};
    c2.gpio_num = config_.in2;
    c2.speed_mode = Mode;
    c2.channel = config_.in2Channel;
    c2.intr_type = LEDC_INTR_DISABLE;
    c2.timer_sel = Timer;
    c2.duty = 0;
    c2.hpoint = 0;
    ledc_channel_config(&c2);
    stop();
    gpio_set_level(config_.sleep, 1);
}
void Drv8837Motor::setDuty(ledc_channel_t ch, uint32_t duty) {
    duty = std::min(duty, MaxDuty);
    ledc_set_duty(Mode, ch, duty);
    ledc_update_duty(Mode, ch);
}
void Drv8837Motor::setSpeed(float speed) {
    speed = std::clamp(speed, -1.0f, 1.0f);
    uint32_t duty = static_cast<uint32_t>(std::fabs(speed) * MaxDuty);
    if (speed > 0) {
        setDuty(config_.in1Channel, duty);
        setDuty(config_.in2Channel, 0);
    } else if (speed < 0) {
        setDuty(config_.in1Channel, 0);
        setDuty(config_.in2Channel, duty);
    } else
        stop();
}
void Drv8837Motor::forward(float p) {
    setSpeed(std::clamp(p, 0.0f, 1.0f));
}
void Drv8837Motor::reverse(float p) {
    setSpeed(-std::clamp(p, 0.0f, 1.0f));
}
void Drv8837Motor::stop() {
    setDuty(config_.in1Channel, 0);
    setDuty(config_.in2Channel, 0);
}
