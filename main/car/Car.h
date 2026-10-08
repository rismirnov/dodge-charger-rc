#pragma once
#include <cstdint>
#include "../lights/ILights.h"
#include "../motor/IMotor.h"
#include "../steering/ISteering.h"

class Car {
   public:
    Car(ISteering& steering, IMotor& motor, ILights& lights);
    void steer(float position);
    void forward(float power);
    void reverse(float power);
    void stop();
    void stopImmediate();
    void centerSteering();
    void setHeadlights(bool enabled);
    void setRearLights(bool enabled);
    void lightsOff();
    void update();

   private:
    enum class Direction { Stopped, Forward, Reverse };
    void requestDrive(Direction direction, float power);
    void startRamp(float targetPower);
    void startStopRamp();
    void applyMotor();
    static uint32_t nowMs();
    ISteering& steering_;
    IMotor& motor_;
    ILights& lights_;
    Direction direction_ = Direction::Stopped, requestedDirection_ = Direction::Stopped;
    float currentPower_ = 0, targetPower_ = 0;
    bool ramping_ = false;
    float rampFromPower_ = 0, rampToPower_ = 0;
    uint32_t rampStartMs_ = 0;
    static constexpr uint32_t StartTimeMs = 250, StopTimeMs = 180;
    static constexpr float PowerEpsilon = 0.001f;
};
