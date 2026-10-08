#pragma once
class IMotor {
   public:
    virtual ~IMotor() = default;
    virtual void init() = 0;
    virtual void forward(float power) = 0;
    virtual void reverse(float power) = 0;
    virtual void stop() = 0;
};
