#include "Car.h"
#include <algorithm>
#include "esp_timer.h"

Car::Car(ISteering& s, IMotor& m, ILights& l) : steering_(s), motor_(m), lights_(l) {}
uint32_t Car::nowMs() {
    return static_cast<uint32_t>(esp_timer_get_time() / 1000ULL);
}
void Car::steer(float p) {
    steering_.setPosition(p);
}
void Car::forward(float p) {
    requestDrive(Direction::Forward, p);
}
void Car::reverse(float p) {
    requestDrive(Direction::Reverse, p);
}
void Car::stop() {
    requestedDirection_ = Direction::Stopped;
    targetPower_ = 0;
    if (currentPower_ > PowerEpsilon)
        startStopRamp();
    else {
        currentPower_ = 0;
        direction_ = Direction::Stopped;
        ramping_ = false;
        motor_.stop();
        lightsOff();
    }
}
void Car::stopImmediate() {
    motor_.stop();
    currentPower_ = targetPower_ = 0;
    direction_ = requestedDirection_ = Direction::Stopped;
    ramping_ = false;
    lightsOff();
}
void Car::centerSteering() {
    steering_.center();
}
void Car::setHeadlights(bool e) {
    lights_.setHeadlights(e);
}
void Car::setRearLights(bool e) {
    lights_.setRearLights(e);
}
void Car::lightsOff() {
    lights_.off();
}
void Car::requestDrive(Direction d, float p) {
    p = std::clamp(p, 0.0f, 1.0f);
    if (p <= PowerEpsilon) {
        stop();
        return;
    }
    requestedDirection_ = d;
    targetPower_ = p;
    if (direction_ == d && currentPower_ > PowerEpsilon) {
        ramping_ = false;
        currentPower_ = targetPower_;
        applyMotor();
        return;
    }
    if (currentPower_ <= PowerEpsilon) {
        direction_ = d;
        currentPower_ = 0;
        startRamp(targetPower_);
        if (direction_ == Direction::Forward) {
            setHeadlights(true);
            setRearLights(false);
        } else {
            setHeadlights(false);
            setRearLights(true);
        }
        return;
    }
    startStopRamp();
}
void Car::startRamp(float p) {
    ramping_ = true;
    rampFromPower_ = currentPower_;
    rampToPower_ = p;
    rampStartMs_ = nowMs();
}
void Car::startStopRamp() {
    if (currentPower_ <= PowerEpsilon) {
        currentPower_ = 0;
        ramping_ = false;
        motor_.stop();
        if (requestedDirection_ == Direction::Stopped) {
            direction_ = Direction::Stopped;
            lightsOff();
        }
        return;
    }
    ramping_ = true;
    rampFromPower_ = currentPower_;
    rampToPower_ = 0;
    rampStartMs_ = nowMs();
}
void Car::update() {
    if (requestedDirection_ != direction_ && currentPower_ > PowerEpsilon && !ramping_)
        startStopRamp();
    if (!ramping_)
        return;
    const uint32_t elapsed = nowMs() - rampStartMs_;
    const bool stopping = rampToPower_ < rampFromPower_;
    const uint32_t duration = stopping ? StopTimeMs : StartTimeMs;
    float progress = static_cast<float>(elapsed) / static_cast<float>(duration);
    progress = std::clamp(progress, 0.0f, 1.0f);
    currentPower_ = rampFromPower_ + (rampToPower_ - rampFromPower_) * progress;
    applyMotor();
    if (progress < 1)
        return;
    currentPower_ = rampToPower_;
    ramping_ = false;
    if (currentPower_ <= PowerEpsilon) {
        currentPower_ = 0;
        motor_.stop();
        if (requestedDirection_ != Direction::Stopped) {
            direction_ = requestedDirection_;
            if (direction_ == Direction::Forward) {
                setHeadlights(true);
                setRearLights(false);
            } else if (direction_ == Direction::Reverse) {
                setHeadlights(false);
                setRearLights(true);
            }
            startRamp(targetPower_);
        } else {
            direction_ = Direction::Stopped;
            lightsOff();
        }
        return;
    }
    applyMotor();
}
void Car::applyMotor() {
    switch (direction_) {
        case Direction::Forward:
            motor_.forward(currentPower_);
            break;
        case Direction::Reverse:
            motor_.reverse(currentPower_);
            break;
        case Direction::Stopped:
            motor_.stop();
            break;
    }
}
