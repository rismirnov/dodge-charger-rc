
#include "Car.h"
#include "audio/SoundManager.h"
#include <algorithm>

Car::Car(ISteering& steering, IMotor& motor, ILights& lights, ISoundManager& sound)
    : steering_(steering), motor_(motor), lights_(lights), sound_(sound) {}

void Car::steer(float position) {
    steering_.setPosition(position);
}

void Car::forward(float power) {
    requestedDirection_ = Direction::Forward;
    targetPower_ = std::clamp(power, 0.0f, 1.0f);
    sound_.playCarMovement();
}

void Car::reverse(float power) {
    requestedDirection_ = Direction::Reverse;
    targetPower_ = std::clamp(power, 0.0f, 1.0f);
    sound_.playCarMovement();
}

void Car::stop() {
    requestedDirection_ = Direction::Stopped;
    targetPower_ = 0.0f;
    sound_.stopCarMovement();
}

void Car::stopImmediate() {
    requestedDirection_ = Direction::Stopped;
    direction_ = Direction::Stopped;
    targetPower_ = 0.0f;
    currentPower_ = 0.0f;

    motor_.stop();
    lightsOff();

    sound_.stopCarMovement();
    wasMoving_ = false;
}

void Car::centerSteering() {
    steering_.center();
}

void Car::setHeadlights(bool enabled) {
    lights_.setHeadlights(enabled);
}

void Car::setRearLights(bool enabled) {
    lights_.setRearLights(enabled);
}

void Car::lightsOff() {
    lights_.off();
}

void Car::updatePowerFilter() {
    const float activeTarget = requestedDirection_ == direction_ ? targetPower_ : 0.0f;

    if (currentPower_ < activeTarget) {
        currentPower_ = std::min(currentPower_ + MaxStepAccel, activeTarget);
    } else if (currentPower_ > activeTarget) {
        currentPower_ = std::max(currentPower_ - MaxStepBrake, activeTarget);
    }
}

void Car::updateLightsForDirection() {
    setHeadlights(true);

    if (direction_ == Direction::Forward) {
        setRearLights(false);
    } else if (direction_ == Direction::Reverse) {
        setRearLights(true);
    }
}

void Car::handleDirectionSwitch() {
    currentPower_ = 0.0f;
    motor_.stop();

    const bool directionChanged = requestedDirection_ != direction_;

    const bool shouldDrive = requestedDirection_ != Direction::Stopped;

    if (shouldDrive && directionChanged) {
        direction_ = requestedDirection_;
        updateLightsForDirection();
    } else if (requestedDirection_ == Direction::Stopped) {
        direction_ = Direction::Stopped;
        setRearLights(false);
    }
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

void Car::update() {
    // Запускаем звук двигателя один раз.
    if (!soundInitialized_) {
        sound_.playEngineStart();
        soundInitialized_ = true;
    }

    updatePowerFilter();

    if (currentPower_ > PowerEpsilon) {
        applyMotor();
    } else {
        handleDirectionSwitch();
    }
    
    wasMoving_ = currentPower_ > PowerEpsilon;
}