idf_component_register(
    SRCS
        "main.cpp"
        "car/Car.cpp"
        "controller/Ds4Controller.cpp"
        "lights/GpioLights.cpp"
        "motor/Drv8837Motor.cpp"
        "steering/ServoSteering.cpp"
        "audio/AudioPlayer.cpp"
        "audio/SoundManager.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        bluepad32
        btstack
        driver
        freertos
)

littlefs_create_partition_image(storage "${CMAKE_SOURCE_DIR}/data" FLASH_IN_PROJECT)