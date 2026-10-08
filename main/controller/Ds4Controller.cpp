#include "Ds4Controller.h"
#include <algorithm>
#include "freertos/FreeRTOS.h"
#include "freertos/portmacro.h"

namespace {
    portMUX_TYPE StateMux = portMUX_INITIALIZER_UNLOCKED;
}

float Ds4Controller::normalizeAxis(int32_t value) {
    constexpr float Max = 512.0f;
    return std::clamp(static_cast<float>(value) / Max, -1.0f, 1.0f);
}

void Ds4Controller::connect(uni_hid_device_t* d) {
    portENTER_CRITICAL(&StateMux);
    device_ = d;
    state_ = {};
    state_.connected = true;
    previousButtonA_ = previousButtonB_ = previousButtonX_ = previousButtonY_ = false;
    portEXIT_CRITICAL(&StateMux);
}

void Ds4Controller::disconnect(uni_hid_device_t* d) {
    portENTER_CRITICAL(&StateMux);
    if (device_ != d && d != nullptr) {
        portEXIT_CRITICAL(&StateMux);
        return;
    }
    device_ = nullptr;
    state_ = {};
    previousButtonA_ = previousButtonB_ = previousButtonX_ = previousButtonY_ = false;
    portEXIT_CRITICAL(&StateMux);
}

void Ds4Controller::onControllerData(uni_hid_device_t* d, const uni_controller_t* c) {
    if (!c || c->klass != UNI_CONTROLLER_CLASS_GAMEPAD)
        return;
    portENTER_CRITICAL(&StateMux);
    if (device_ != d) {
        portEXIT_CRITICAL(&StateMux);
        return;
    }
    const uni_gamepad_t& g = c->gamepad;
    const bool a = (g.buttons & BUTTON_A) != 0, b = (g.buttons & BUTTON_B) != 0, x = (g.buttons & BUTTON_X) != 0,
               y = (g.buttons & BUTTON_Y) != 0;
    state_.steering = normalizeAxis(g.axis_x);
    const float drive = normalizeAxis(g.axis_ry);
    if (drive < 0) {
        state_.throttle = -drive;
        state_.reverse = 0;
    } else {
        state_.throttle = 0;
        state_.reverse = drive;
    }
    state_.connected = true;
    state_.buttonA = a;
    state_.buttonB = b;
    state_.buttonX = x;
    state_.buttonY = y;
    if (x && !previousButtonX_)
        state_.headlights = true;
    if (y && !previousButtonY_)
        state_.headlights = false;
    if (a && !previousButtonA_)
        state_.rearLights = true;
    if (b && !previousButtonB_)
        state_.rearLights = false;
    previousButtonA_ = a;
    previousButtonB_ = b;
    previousButtonX_ = x;
    previousButtonY_ = y;
    const uint8_t dpad = g.dpad;
    state_.dpadUp = (dpad & DPAD_UP) != 0;
    state_.dpadDown = (dpad & DPAD_DOWN) != 0;
    state_.dpadLeft = (dpad & DPAD_LEFT) != 0;
    state_.dpadRight = (dpad & DPAD_RIGHT) != 0;
    portEXIT_CRITICAL(&StateMux);
}

ControllerState Ds4Controller::getState() const {
    ControllerState s;
    portENTER_CRITICAL(&StateMux);
    s = state_;
    portEXIT_CRITICAL(&StateMux);
    return s;
}
