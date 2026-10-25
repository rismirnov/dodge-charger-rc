#pragma once

#include "esp_audio_simple_player.h"

#include <atomic>
#include <string>

class AudioPlayer {
public:
    AudioPlayer(int bclk_pin, int ws_pin, int din_pin);
    ~AudioPlayer();

    AudioPlayer(const AudioPlayer&) = delete;
    AudioPlayer& operator=(const AudioPlayer&) = delete;

    /** Stops current playback, then starts the WAV from LittleFS. */
    bool play(const std::string& filename);
    void stop();

    void setVolume(int volume);
    bool isPlaying() const;

private:
    bool initI2S();
    bool initPlayer();

    static int audioEventCallback(esp_asp_event_pkt_t* pkt, void* ctx);
    static int i2sWriteCallback(uint8_t* data, int data_size, void* ctx);
    void handlePlayerEvent(esp_asp_event_pkt_t* pkt);

    std::string uriForFilename(const std::string& filename) const;
    void ensureI2sOutputEnabled();
    void disableI2sOutput();

    int m_bclk_pin;
    int m_ws_pin;
    int m_din_pin;

    esp_asp_handle_t m_player = nullptr;
    int m_i2s_port = 0;

    std::atomic<bool> m_is_playing{false};
    std::atomic<int> m_volume{100};
    bool m_i2s_enabled = false;
};
