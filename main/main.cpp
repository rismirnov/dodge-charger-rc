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
#include "steering/ServoSteering.h"
#include "audio/AudioPlayer.h"
#include "audio/SoundManager.h"
#include "esp_littlefs.h"

namespace {
constexpr char TAG[] = "Rastar";
constexpr float kDriveDeadzone = 0.05f;
constexpr TickType_t kCarTaskPeriod = pdMS_TO_TICKS(10);
constexpr int kLogEveryNTicks = 50;

Ds4Controller controller;

struct CarTaskContext {
    Car* car;
    GpioLights* lights;
};

static void applyControllerToCar(Car& car, const ControllerState& state, bool& wasConnected) {
    if (!state.connected) {
        if (wasConnected) {
            car.stopImmediate();
            car.centerSteering();
        }
        wasConnected = false;
        return;
    }

    wasConnected = true;
    car.steer(state.steering);

    if (state.reverse > kDriveDeadzone) {
        car.reverse(state.reverse);
    } else if (state.throttle > kDriveDeadzone) {
        car.forward(state.throttle);
    } else {
        car.stop();
    }
}

static void carTask(void* param) {
    auto* ctx = static_cast<CarTaskContext*>(param);
    Car& car = *ctx->car;
    GpioLights& lights = *ctx->lights;

    bool wasConnected = false;
    int logCounter = 0;

    for (;;) {
        const ControllerState state = controller.getState();
        applyControllerToCar(car, state, wasConnected);

        car.update();
        lights.update();

        if (++logCounter >= kLogEveryNTicks) {
            logCounter = 0;
            ESP_LOGI(TAG, "connected=%d steering=%.2f throttle=%.2f reverse=%.2f", state.connected,
                     state.steering, state.throttle, state.reverse);
        }

        vTaskDelay(kCarTaskPeriod);
    }
}

static bool mountLittleFs() {
    ESP_LOGI(TAG, "Mounting LittleFS from internal Flash...");

    esp_vfs_littlefs_conf_t conf = {
        .base_path = "/littlefs",
        .partition_label = "storage",
        .format_if_mount_failed = false,
        .dont_mount = false,
    };

    const esp_err_t ret = esp_vfs_littlefs_register(&conf);
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "LittleFS mounted successfully.");
        return true;
    }

    if (ret == ESP_FAIL) {
        ESP_LOGE(TAG, "Failed to mount LittleFS partition.");
    } else if (ret == ESP_ERR_NOT_FOUND) {
        ESP_LOGE(TAG, "Partition with label 'storage' not found.");
    } else {
        ESP_LOGE(TAG, "LittleFS initialization error: %s", esp_err_to_name(ret));
    }
    return false;
}

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

static uni_platform* rastarPlatform() {
    static uni_platform platform{
        .name = "rastar",
        .init = platformInit,
        .on_init_complete = platformOnInitComplete,
        .on_device_discovered = platformOnDeviceDiscovered,
        .on_device_connected = platformOnDeviceConnected,
        .on_device_disconnected = platformOnDeviceDisconnected,
        .on_device_ready = platformOnDeviceReady,
        .on_controller_data = platformOnControllerData,
        .get_property = platformGetProperty,
        .on_oob_event = platformOnOobEvent,
    };
    return &platform;
}

}  // namespace

extern "C" void app_main(void) {
    (void)mountLittleFs();

    AudioPlayer player(Pins::AudioBclk, Pins::AudioWs, Pins::AudioDin);
    SoundManager soundManager(player);
    player.setVolume(50);

    static ServoSteering steering({
        .pin = Pins::SteeringServo,
        .channel = Pins::SteeringChannel,
    });

    static Drv8837Motor motor({
        .in1 = Pins::MotorIn1,
        .in2 = Pins::MotorIn2,
        .in1Channel = Pins::MotorIn1Channel,
        .in2Channel = Pins::MotorIn2Channel,
        .pwmFrequency = 25000,
    });

    static GpioLights lights({
        .headlights = Pins::Headlights,
        .rearLights = Pins::RearLights,
    });

    static Car car(steering, motor, lights, soundManager);

    steering.init();
    motor.init();
    lights.init();

    car.stop();
    car.centerSteering();

    CarTaskContext carTaskCtx{&car, &lights};
    xTaskCreate(carTask, "car", 4096, &carTaskCtx, 5, nullptr);

    btstack_init();
    uni_platform_set_custom(rastarPlatform());
    uni_init(0, nullptr);

    ESP_LOGI(TAG, "Rastar RC ready");
    btstack_run_loop_execute();
}
