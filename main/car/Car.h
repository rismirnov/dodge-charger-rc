#pragma once

#include <cstdint>
#include "steering/ISteering.h" // Укажите ваши правильные пути к интерфейсам
#include "motor/IMotor.h"
#include "lights/ILights.h"
#include "audio/ISoundManager.h"

class Car {
public:
    enum class Direction {
        Stopped,
        Forward,
        Reverse
    };

    Car(ISteering& s, IMotor& m, ILights& l, ISoundManager& sound);

    void steer(float p);
    void forward(float p);
    void reverse(float p);
    void stop();
    void stopImmediate();
    
    void centerSteering();
    void setHeadlights(bool e);
    void setRearLights(bool e);
    void lightsOff();

    void update(); // Вызывается каждые 10 мс из carTask

private:
    void applyMotor();
    void updatePowerFilter();       // Расчет плавного изменения мощности
    void handleDirectionSwitch();    // Логика переключения направления на нулевой скорости
    void updateLightsForDirection(); // Автоматическое переключение фар

    ISteering& steering_;
    IMotor& motor_;
    ILights& lights_;
    ISoundManager& sound_;

    bool wasMoving_ = false;
    bool soundInitialized_ = false;
    Direction direction_ = Direction::Stopped;
    Direction requestedDirection_ = Direction::Stopped;

    float currentPower_ = 0.0f;
    float targetPower_ = 0.0f;

    static constexpr float PowerEpsilon = 0.02f;
    
    // Коэффициенты плавности (изменение мощности за 1 шаг в 10 мс)
    // 0.05f означает, что от 0 до 100% машина разгонится за 20 шагов (200 мс)
    static constexpr float MaxStepAccel = 0.05f; // Плавность разгона
    static constexpr float MaxStepBrake = 0.10f; // Плавность торможения
};