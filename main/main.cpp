#include <uni.h>

#include "esp_log.h"

#include "btstack_port_esp32.h"
#include "btstack_run_loop.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "car/Car.h"
#include "config/Pins.h"
#include "controller/Ds4Controller.h"
#include "lights/GpioLights.h"
#include "motor/Drv8837Motor.h"
#include "steering/Sg90Steering.h"

namespace {
    constexpr char TAG[] = "Rastar";

    Ds4Controller controller;
    Car* carInstance = nullptr;

}  // namespace

//
// Bluepad32 platform callbacks
//

static void platformInit(int argc, const char** argv) {
    (void)argc;
    (void)argv;

    ESP_LOGI(TAG, "Bluepad32 platform init");
}

static void platformOnInitComplete() {
    ESP_LOGI(TAG, "Bluepad32 initialized");

    uni_bt_start_scanning_and_autoconnect_unsafe();
    uni_bt_allow_incoming_connections(true);
}

static uni_error_t platformOnDeviceDiscovered(bd_addr_t addr, const char* name, uint16_t cod, uint8_t rssi) {
    (void)addr;
    (void)name;
    (void)cod;
    (void)rssi;

    return UNI_ERROR_SUCCESS;
}

static void platformOnDeviceConnected(uni_hid_device_t* device) {
    ESP_LOGI(TAG, "Controller connected: %p", device);
}

static void platformOnDeviceDisconnected(uni_hid_device_t* device) {
    controller.disconnect(device);

    ESP_LOGI(TAG, "Controller disconnected: %p", device);
}

static uni_error_t platformOnDeviceReady(uni_hid_device_t* device) {
    controller.connect(device);

    ESP_LOGI(TAG, "Controller ready: %p", device);

    return UNI_ERROR_SUCCESS;
}

static void platformOnControllerData(uni_hid_device_t* device, uni_controller_t* data) {
    controller.onControllerData(device, data);
}

static const uni_property_t* platformGetProperty(uni_property_idx_t idx) {
    (void)idx;

    return nullptr;
}

static void platformOnOobEvent(uni_platform_oob_event_t event, void* data) {
    (void)data;

    if (event == UNI_PLATFORM_OOB_BLUETOOTH_ENABLED) {
        ESP_LOGI(TAG, "Bluetooth enabled");
    }
}

static struct uni_platform* getMyPlatform() {
    static struct uni_platform platform{};

    platform.name = "rastar";
    platform.init = platformInit;
    platform.on_init_complete = platformOnInitComplete;
    platform.on_device_discovered = platformOnDeviceDiscovered;
    platform.on_device_connected = platformOnDeviceConnected;
    platform.on_device_disconnected = platformOnDeviceDisconnected;
    platform.on_device_ready = platformOnDeviceReady;
    platform.on_oob_event = platformOnOobEvent;
    platform.on_controller_data = platformOnControllerData;
    platform.get_property = platformGetProperty;

    return &platform;
}

//
// Car task
//

static void carTask(void*) {
    bool wasConnected = false;

    for (;;) {
        if (!carInstance) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        const ControllerState state = controller.getState();

        if (!state.connected) {
            if (wasConnected) {
                carInstance->stopImmediate();
                carInstance->centerSteering();
            }

            wasConnected = false;
        } else {
            wasConnected = true;

            carInstance->steer(state.steering);

            if (state.reverse > 0.05f) {
                carInstance->reverse(state.reverse);
            } else if (state.throttle > 0.05f) {
                carInstance->forward(state.throttle);
            } else {
                carInstance->stop();
            }
        }

        carInstance->update();

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

//
// ESP-IDF entry point
//

extern "C" void app_main(void) {
    static Sg90Steering steering({
        .pin = Pins::SteeringServo,
        .channel = Pins::SteeringChannel,
    });

    static Drv8837Motor motor({
        .in1 = Pins::MotorIn1,
        .in2 = Pins::MotorIn2,
        .sleep = Pins::MotorSleep,
        .in1Channel = Pins::MotorIn1Channel,
        .in2Channel = Pins::MotorIn2Channel,
        .pwmFrequency = 20000,
    });

    static GpioLights lights({
        .headlights = Pins::Headlights,
        .rearLights = Pins::RearLights,
    });

    static Car car(steering, motor, lights);

    carInstance = &car;

    steering.init();
    motor.init();
    lights.init();

    car.stop();
    car.centerSteering();

    xTaskCreate(carTask, "car", 4096, nullptr, 5, nullptr);

    btstack_init();

    uni_platform_set_custom(getMyPlatform());

    uni_init(0, nullptr);

    ESP_LOGI(TAG, "Rastar RC ready");

    btstack_run_loop_execute();
}