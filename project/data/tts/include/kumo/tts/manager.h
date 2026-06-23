#pragma once

#include "kumo/tts/queue.h"
#include "kumo/config/config.h"
#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <deque>
#include <mutex>
#include <functional>

namespace kumo {

// ============================================================================
// TtsCommand — commands sent to the TTS worker thread
// ============================================================================

enum class TtsCommandType {
    Speak,
    Stop,
    SetVolume,
    ClearQueue,
    Quit,
};

struct TtsCommand {
    TtsCommandType type;
    std::string text;
    float volume = 1.0f;
};

// ============================================================================
// TtsManager — manages TTS output across providers
// ============================================================================

class TtsManager {
public:
    explicit TtsManager(std::shared_ptr<ConfigStore> config_store);
    ~TtsManager();

    TtsManager(const TtsManager&) = delete;
    TtsManager& operator=(const TtsManager&) = delete;

    void speak(const std::string& text, int32_t priority = 0);
    void stop();
    void set_volume(float volume);
    void clear_queue();

private:
    std::shared_ptr<ConfigStore> m_config;
    TtsQueue m_queue;
    std::deque<TtsCommand> m_commands;
    std::mutex m_cmd_mutex;
    std::thread m_worker;
    std::atomic<bool> m_running{false};

    void worker_loop();
    void process_command(const TtsCommand& cmd);
    void speak_system(const std::string& text);
};

} // namespace kumo
