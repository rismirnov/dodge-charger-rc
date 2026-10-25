#pragma once

#include "ISoundManager.h"
#include "AudioPlayer.h"

#include <string>

class SoundManager: public ISoundManager {
public:
    // Конструктор принимает пины I2S, чтобы самому инициализировать плеер
    SoundManager(AudioPlayer& player);
    ~SoundManager() = default;

    // Высокоуровневые методы для озвучки автомобиля
    void playEngineStart();   // Играет один раз
    void playCarMovement();   // Один проигрыш; Car::update перезапускает, пока едем
    void playCarBrake();      // Играет один раз
    void stopCarMovement();

    // Дополнительные полезные методы управления
    void stopAll();
    void setVolume(int volume);
    bool isPlaying() const;

private:
    AudioPlayer& m_player;
    bool m_carMovementPlaying = false;
    bool m_movementStartPending = false;

    // Константы имен файлов в LittleFS (без слэша в начале)
    const std::string FILE_ENGINE_START = "engine_start.wav";
    const std::string FILE_CAR_MOVEMENT = "car_movement.wav";
    const std::string FILE_CAR_BRAKE    = "car_brake.wav";
};