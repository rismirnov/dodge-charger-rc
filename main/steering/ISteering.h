#pragma once
class ISteering {
   public:
    virtual ~ISteering() = default;
    virtual void setPosition(float position) = 0;
    virtual void center() = 0;
};
