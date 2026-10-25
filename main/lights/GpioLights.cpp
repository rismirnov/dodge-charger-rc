
#include "GpioLights.h"

#include <algorithm>

#include "driver/ledc.h"
#include "esp_check.h"
#include "esp_timer.h"

namespace {
constexpr char Tag[] = "GpioLights";

constexpr ledc_mode_t PwmMode = LEDC_LOW_SPEED_MODE;
}

GpioLights::GpioLights(const Config& config)
    : config_(config) {}

void GpioLights::init() {
    ledc_timer_config_t timerConfig{};
    timerConfig.speed_mode = PwmMode;
    timerConfig.duty_resolution = PwmResolution;
    timerConfig.timer_num = Pins::LightsTimer;
    timerConfig.freq_hz = PwmFrequencyHz;
    timerConfig.clk_cfg = LEDC_AUTO_CLK;

    ESP_ERROR_CHECK(ledc_timer_config(&timerConfig));

    const auto initChannel = [](gpio_num_t pin,
                                ledc_channel_t channel) {
        ledc_channel_config_t channelConfig{};
        channelConfig.gpio_num = pin;
        channelConfig.speed_mode = PwmMode;
        channelConfig.channel = channel;
        channelConfig.intr_type = LEDC_INTR_DISABLE;
        channelConfig.timer_sel = Pins::LightsTimer;
        channelConfig.duty = 0;
        channelConfig.hpoint = 0;

        ESP_ERROR_CHECK(ledc_channel_config(&channelConfig));
    };

    initChannel(config_.headlights, Pins::HeadlightsChannel);
    initChannel(config_.rearLights, Pins::RearLightsChannel);

    headlights_ = {};
    rearLights_ = {};
    lastUpdateUs_ = esp_timer_get_time();

    off();
}

void GpioLights::setTarget(Light& light, bool enabled) {
    light.target = enabled ? 1.0f : 0.0f;
}

void GpioLights::setHeadlights(bool enabled) {
    setTarget(headlights_, enabled);
}

void GpioLights::setRearLights(bool enabled) {
    setTarget(rearLights_, enabled);
}

void GpioLights::off() {
    setHeadlights(false);
    setRearLights(false);
}

void GpioLights::updateLight(
    Light& light,
    ledc_channel_t channel,
    float deltaSeconds
) {
    if (light.current == light.target) {
        return;
    }

    const float step =
        deltaSeconds * 1000.0f / FadeDurationMs;

    if (light.current < light.target) {
        light.current = std::min(
            light.current + step, light.target);
    } else if (light.current > light.target) {
        light.current = std::max(
            light.current - step, light.target);
    }

    // Квадратичная коррекция для более естественного
    // визуального восприятия яркости.
    const float brightness = light.current * light.current;

    const uint32_t duty = static_cast<uint32_t>(
        brightness * MaxDuty + 0.5f);

    ESP_ERROR_CHECK(
        ledc_set_duty(PwmMode, channel, duty));

    ESP_ERROR_CHECK(
        ledc_update_duty(PwmMode, channel));
}

void GpioLights::update() {
    const int64_t nowUs = esp_timer_get_time();

    const float deltaSeconds = std::clamp(
        static_cast<float>(nowUs - lastUpdateUs_) / 1'000'000.0f,
        0.0f,
        0.1f
    );

    lastUpdateUs_ = nowUs;

    updateLight(
        headlights_,
        Pins::HeadlightsChannel,
        deltaSeconds
    );

    updateLight(
        rearLights_,
        Pins::RearLightsChannel,
        deltaSeconds
    );
}
