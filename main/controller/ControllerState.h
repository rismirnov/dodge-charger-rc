#pragma once
struct ControllerState {
    bool connected = false;
    float steering = 0.0f;
    float throttle = 0.0f;
    float reverse = 0.0f;
    bool headlights = false;
    bool rearLights = false;
    bool buttonA = false;
    bool buttonB = false;
    bool buttonX = false;
    bool buttonY = false;
    bool dpadUp = false;
    bool dpadDown = false;
    bool dpadLeft = false;
    bool dpadRight = false;
};
