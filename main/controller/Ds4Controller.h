#pragma once
#include <uni.h>
#include <cstdint>
#include "ControllerState.h"

class Ds4Controller {
   public:
    void connect(uni_hid_device_t* device);
    void disconnect(uni_hid_device_t* device);
    void onControllerData(uni_hid_device_t* device, const uni_controller_t* controller);
    ControllerState getState() const;

   private:
    static float normalizeAxis(int32_t value);
    uni_hid_device_t* device_ = nullptr;
    ControllerState state_{};
    bool previousButtonA_ = false, previousButtonB_ = false, previousButtonX_ = false, previousButtonY_ = false;
};
