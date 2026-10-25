#include "SoundManager.h"
#include "AudioPlayer.h"
#include "esp_log.h"

static const char* TAG = "SOUND_MANAGER";

SoundManager::SoundManager(AudioPlayer& player) : m_player(player) {}

void SoundManager::playEngineStart() {
    ESP_LOGI(TAG, "Engine start");
    if (!m_player.play(FILE_ENGINE_START)) {
        ESP_LOGE(TAG, "Failed: %s", FILE_ENGINE_START.c_str());
    }
}

void SoundManager::playCarMovement() {
    if (m_player.isPlaying()) {
        m_carMovementPlaying = true;
        return;
    }

    if (m_movementStartPending) {
        return;
    }

    m_movementStartPending = true;
    m_carMovementPlaying = true;

    ESP_LOGI(TAG, "Car movement");
    const bool ok = m_player.play(FILE_CAR_MOVEMENT);
    m_movementStartPending = false;

    if (!ok) {
        ESP_LOGE(TAG, "Failed: %s", FILE_CAR_MOVEMENT.c_str());
        m_carMovementPlaying = false;
    }
}

void SoundManager::stopCarMovement() {
    if (!m_carMovementPlaying) {
        return;
    }

    ESP_LOGI(TAG, "Car movement stop");
    m_carMovementPlaying = false;
    m_movementStartPending = false;
    m_player.stop();
}

void SoundManager::playCarBrake() {
    ESP_LOGI(TAG, "Car brake");
    m_carMovementPlaying = false;
    if (!m_player.play(FILE_CAR_BRAKE)) {
        ESP_LOGE(TAG, "Failed: %s", FILE_CAR_BRAKE.c_str());
    }
}

void SoundManager::stopAll() {
    m_carMovementPlaying = false;
    m_player.stop();
}

void SoundManager::setVolume(int volume) {
    m_player.setVolume(volume);
}

bool SoundManager::isPlaying() const {
    return m_player.isPlaying();
}
