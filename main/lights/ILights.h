#pragma once
class ILights {
public:
    virtual ~ILights() = default;
    virtual void setHeadlights(bool enabled) = 0;
    virtual void setRearLights(bool enabled) = 0;
    virtual void off() = 0;
};
