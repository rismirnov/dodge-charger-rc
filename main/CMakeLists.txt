idf_component_register(
    SRCS
        "main.cpp"
        "car/Car.cpp"
        "controller/Ds4Controller.cpp"
        "lights/GpioLights.cpp"
        "motor/Drv8837Motor.cpp"
        "steering/Sg90Steering.cpp"

    INCLUDE_DIRS
        "."

    REQUIRES
        bluepad32
        btstack
)