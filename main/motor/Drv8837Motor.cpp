
#include "Drv8837Motor.h"

#include <algorithm>
#include <cmath>

#include "esp_check.h"

namespace {
constexpr ledc_timer_t Timer = LEDC_TIMER_1;
constexpr ledc_mode_t Mode = LEDC_LOW_SPEED_MODE;
constexpr ledc_timer_bit_t Resolution = LEDC_TIMER_10_BIT;
constexpr uint32_t MaxDuty = (1u << 10) - 1;
constexpr float MotorPowerOffset = 0.62;
}  // namespace

Drv8837Motor::Drv8837Motor(const Config& config) : config_(config) {}

void Drv8837Motor::init() {
    // nSLEEP подключён непосредственно к 3.3 В.
    // Поэтому GPIO для управления им не настраиваем.

    ledc_timer_config_t timer{};
    timer.speed_mode = Mode;
    timer.timer_num = Timer;
    timer.duty_resolution = Resolution;
    timer.freq_hz = config_.pwmFrequency;
    timer.clk_cfg = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    ledc_channel_config_t channel1{};
    channel1.gpio_num = config_.in1;
    channel1.speed_mode = Mode;
    channel1.channel = config_.in1Channel;
    channel1.intr_type = LEDC_INTR_DISABLE;
    channel1.timer_sel = Timer;
    channel1.duty = 0;
    channel1.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channel1));

    ledc_channel_config_t channel2{};
    channel2.gpio_num = config_.in2;
    channel2.speed_mode = Mode;
    channel2.channel = config_.in2Channel;
    channel2.intr_type = LEDC_INTR_DISABLE;
    channel2.timer_sel = Timer;
    channel2.duty = 0;
    channel2.hpoint = 0;

    ESP_ERROR_CHECK(ledc_channel_config(&channel2));

    stop();
}

void Drv8837Motor::setDuty(ledc_channel_t channel, uint32_t duty) {
    duty = std::min(duty, MaxDuty);

    ESP_ERROR_CHECK(ledc_set_duty(Mode, channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(Mode, channel));
}

void Drv8837Motor::setSpeed(float speed) {
    speed = std::clamp(speed, -1.0f, 1.0f);

    const float normSpeed = MotorPowerOffset + std::fabs(speed) * (1.0f - MotorPowerOffset);
    const uint32_t duty = static_cast<uint32_t>(normSpeed * MaxDuty);

    if (speed > 0.0f) {
        // Сначала выключаем обратное направление.
        setDuty(config_.in2Channel, 0);
        setDuty(config_.in1Channel, duty);
    } else if (speed < 0.0f) {
        // Сначала выключаем прямое направление.
        setDuty(config_.in1Channel, 0);
        setDuty(config_.in2Channel, duty);
    } else {
        stop();
    }
}

void Drv8837Motor::forward(float power) {
    setSpeed(std::clamp(power, 0.0f, 1.0f));
}

void Drv8837Motor::reverse(float power) {
    setSpeed(-std::clamp(power, 0.0f, 1.0f));
}

void Drv8837Motor::stop() {
    setDuty(config_.in1Channel, 0);
    setDuty(config_.in2Channel, 0);
}
