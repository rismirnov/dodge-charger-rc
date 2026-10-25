#include "AudioPlayer.h"
#include "freertos/FreeRTOS.h"
#include "driver/gpio.h"
#include "driver/i2s_common.h"
#include "driver/i2s_std.h"
#include "esp_log.h"

#include <cstring>

static const char* TAG = "AUDIO_PLAYER";
static i2s_chan_handle_t tx_handle = nullptr;

AudioPlayer::AudioPlayer(int bclk_pin, int ws_pin, int din_pin)
    : m_bclk_pin(bclk_pin), m_ws_pin(ws_pin), m_din_pin(din_pin) {
    if (initI2S() && initPlayer()) {
        ESP_LOGI(TAG, "AudioPlayer ready (48 kHz mono WAV).");
    } else {
        ESP_LOGE(TAG, "AudioPlayer init failed.");
    }
}

AudioPlayer::~AudioPlayer() {
    if (m_player) {
        esp_audio_simple_player_stop(m_player);
        esp_audio_simple_player_destroy(m_player);
        m_player = nullptr;
    }

    if (tx_handle) {
        i2s_channel_disable(tx_handle);
        i2s_del_channel(tx_handle);
        tx_handle = nullptr;
    }
}

bool AudioPlayer::initI2S() {
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG((i2s_port_t)m_i2s_port, I2S_ROLE_MASTER);
    chan_cfg.dma_desc_num = 8;
    chan_cfg.dma_frame_num = 512;

    if (i2s_new_channel(&chan_cfg, &tx_handle, nullptr) != ESP_OK) {
        ESP_LOGE(TAG, "I2S channel create failed");
        return false;
    }

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(48000),
        .slot_cfg = I2S_STD_MSB_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO),
        .gpio_cfg = {.mclk = I2S_GPIO_UNUSED,
                     .bclk = (gpio_num_t)m_bclk_pin,
                     .ws = (gpio_num_t)m_ws_pin,
                     .dout = (gpio_num_t)m_din_pin,
                     .din = I2S_GPIO_UNUSED,
                     .invert_flags = {.mclk_inv = false, .bclk_inv = false, .ws_inv = false}}};
    std_cfg.clk_cfg.clk_src = I2S_CLK_SRC_APLL;

    if (i2s_channel_init_std_mode(tx_handle, &std_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "I2S init failed");
        i2s_del_channel(tx_handle);
        tx_handle = nullptr;
        return false;
    }

    return true;
}

void AudioPlayer::ensureI2sOutputEnabled() {
    if (!tx_handle || m_i2s_enabled) {
        return;
    }
    if (i2s_channel_enable(tx_handle) == ESP_OK) {
        m_i2s_enabled = true;
    }
}

void AudioPlayer::disableI2sOutput() {
    if (!tx_handle || !m_i2s_enabled) {
        return;
    }

    alignas(4) static uint8_t silence[512];
    for (int i = 0; i < 2; ++i) {
        size_t written = 0;
        i2s_channel_write(tx_handle, silence, sizeof(silence), &written, pdMS_TO_TICKS(20));
    }

    i2s_channel_disable(tx_handle);
    m_i2s_enabled = false;
}

bool AudioPlayer::initPlayer() {
    esp_asp_cfg_t cfg = {};
    cfg.task_prio = 12;
    cfg.task_stack = 4096 * 2;
#if CONFIG_FREERTOS_UNICORE
    cfg.task_core = 0;
#else
    cfg.task_core = 1;
#endif
    cfg.out.cb = AudioPlayer::i2sWriteCallback;
    cfg.out.user_ctx = this;

    if (esp_audio_simple_player_new(&cfg, &m_player) != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "esp_audio_simple_player_new failed");
        return false;
    }

    if (esp_audio_simple_player_set_event(m_player, AudioPlayer::audioEventCallback, this) != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "set_event failed");
        esp_audio_simple_player_destroy(m_player);
        m_player = nullptr;
        return false;
    }

    return true;
}

std::string AudioPlayer::uriForFilename(const std::string& filename) const {
    std::string name = filename;
    if (!name.empty() && name.front() == '/') {
        name.erase(0, 1);
    }
    return "file://littlefs/" + name;
}

bool AudioPlayer::play(const std::string& filename) {
    if (!m_player || filename.empty()) {
        return false;
    }

    const std::string uri = uriForFilename(filename);
    if (uri.size() <= strlen("file://littlefs/")) {
        ESP_LOGE(TAG, "play: empty filename");
        return false;
    }

    esp_audio_simple_player_stop(m_player);
    ensureI2sOutputEnabled();

    ESP_LOGI(TAG, "play: %s", uri.c_str());

    const esp_gmf_err_t err = esp_audio_simple_player_run(m_player, uri.c_str(), nullptr);
    if (err != ESP_GMF_ERR_OK) {
        ESP_LOGE(TAG, "run failed: %d", static_cast<int>(err));
        esp_audio_simple_player_stop(m_player);
        m_is_playing.store(false, std::memory_order_relaxed);
        disableI2sOutput();
        return false;
    }

    // RUNNING event comes later; avoid a second play() from carTask in the same ~20 ms window.
    m_is_playing.store(true, std::memory_order_relaxed);
    return true;
}

void AudioPlayer::stop() {
    if (!m_player) {
        return;
    }

    ESP_LOGI(TAG, "stop");
    esp_audio_simple_player_stop(m_player);
    m_is_playing.store(false, std::memory_order_relaxed);
    disableI2sOutput();
}

int AudioPlayer::i2sWriteCallback(uint8_t* data, int data_size, void* ctx) {
    auto* player = static_cast<AudioPlayer*>(ctx);
    if (!player || !data || data_size <= 0 || !tx_handle || !player->m_i2s_enabled) {
        return 0;
    }

    const int volume = player->m_volume.load(std::memory_order_relaxed);
    if (volume < 100) {
        auto* samples = reinterpret_cast<int16_t*>(data);
        const int n = data_size / static_cast<int>(sizeof(int16_t));
        for (int i = 0; i < n; ++i) {
            int32_t s = static_cast<int32_t>(samples[i]) * volume / 100;
            if (s > 32767) {
                s = 32767;
            }
            if (s < -32768) {
                s = -32768;
            }
            samples[i] = static_cast<int16_t>(s);
        }
    }

    size_t written = 0;
    if (i2s_channel_write(tx_handle, data, static_cast<size_t>(data_size), &written, portMAX_DELAY) != ESP_OK) {
        return 0;
    }
    return static_cast<int>(written);
}

int AudioPlayer::audioEventCallback(esp_asp_event_pkt_t* pkt, void* ctx) {
    auto* player = static_cast<AudioPlayer*>(ctx);
    if (player && pkt) {
        player->handlePlayerEvent(pkt);
    }
    return 0;
}

void AudioPlayer::handlePlayerEvent(esp_asp_event_pkt_t* pkt) {
    if (!pkt || pkt->type != ESP_ASP_EVENT_TYPE_STATE || !pkt->payload ||
        pkt->payload_size < static_cast<int>(sizeof(esp_asp_state_t))) {
        return;
    }

    const auto state = *static_cast<const esp_asp_state_t*>(pkt->payload);

    switch (state) {
        case ESP_ASP_STATE_RUNNING:
            m_is_playing.store(true, std::memory_order_relaxed);
            break;
        case ESP_ASP_STATE_PAUSED:
        case ESP_ASP_STATE_STOPPED:
        case ESP_ASP_STATE_FINISHED:
        case ESP_ASP_STATE_ERROR:
            m_is_playing.store(false, std::memory_order_relaxed);
            break;
        default:
            break;
    }
}

void AudioPlayer::setVolume(int volume) {
    if (volume < 0) {
        volume = 0;
    }
    if (volume > 100) {
        volume = 100;
    }
    m_volume.store(volume, std::memory_order_relaxed);
}

bool AudioPlayer::isPlaying() const {
    return m_is_playing.load(std::memory_order_relaxed);
}
