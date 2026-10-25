#pragma once

#include "esp_err.h"

class ISoundManager {
public:
    virtual ~ISoundManager() = default;

    virtual void playEngineStart() = 0;
    virtual void playCarMovement() = 0;
    virtual void stopCarMovement() = 0;
    virtual void playCarBrake() = 0;
    virtual void stopAll() = 0;
    virtual void setVolume(int volume) = 0;
};