#include "kumo/tts/manager.h"
#include <chrono>

namespace kumo {

TtsManager::TtsManager(std::shared_ptr<ConfigStore> config_store)
    : m_config(std::move(config_store))
    , m_running(true) {
    m_worker = std::thread([this]() { worker_loop(); });
}

TtsManager::~TtsManager() {
    m_running = false;
    {
        std::unique_lock lock(m_cmd_mutex);
        m_commands.push_back({TtsCommandType::Quit});
    }
    if (m_worker.joinable()) {
        m_worker.join();
    }
}

void TtsManager::speak(const std::string& text, int32_t priority) {
    TtsMessage msg;
    msg.text = text;
    msg.priority = priority;
    if (priority < 0) {
        m_queue.push_high_priority(msg);
    } else {
        m_queue.push(msg);
    }
}

void TtsManager::stop() {
    std::unique_lock lock(m_cmd_mutex);
    m_commands.push_back({TtsCommandType::Stop});
}

void TtsManager::set_volume(float volume) {
    std::unique_lock lock(m_cmd_mutex);
    m_commands.push_back({TtsCommandType::SetVolume, "", volume});
}

void TtsManager::clear_queue() {
    m_queue.clear();
    std::unique_lock lock(m_cmd_mutex);
    m_commands.push_back({TtsCommandType::ClearQueue});
}

void TtsManager::worker_loop() {
    while (m_running) {
        // Process commands
        {
            std::unique_lock lock(m_cmd_mutex);
            while (!m_commands.empty()) {
                auto cmd = m_commands.front();
                m_commands.pop_front();
                lock.unlock();
                process_command(cmd);
                lock.lock();
            }
        }

        // Process TTS queue
        auto msg = m_queue.pop();
        if (msg) {
            speak_system(msg->text);
        }

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

void TtsManager::process_command(const TtsCommand& cmd) {
    switch (cmd.type) {
        case TtsCommandType::Quit:
            m_running = false;
            break;
        case TtsCommandType::Stop:
            // Stop current speech
            break;
        case TtsCommandType::ClearQueue:
            m_queue.clear();
            break;
        case TtsCommandType::SetVolume:
            // Change volume
            break;
        default:
            break;
    }
}

void TtsManager::speak_system(const std::string& text) {
    // Platform-specific TTS:
    // Windows: use ISpVoice COM interface (SAPI)
    // macOS: use AVSpeechSynthesizer (via Objective-C++)
    // Linux: use speech-dispatcher or espeak
    (void)text;
}

} // namespace kumo
