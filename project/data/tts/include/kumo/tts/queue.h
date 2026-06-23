#pragma once

#include <string>
#include <deque>
#include <mutex>

namespace kumo {

// ============================================================================
// TtsMessage
// ============================================================================

struct TtsMessage {
    std::string text;
    std::string voice;
    int32_t priority = 0;  // lower = higher priority
    std::string id;
};

// ============================================================================
// TtsQueue
// ============================================================================

class TtsQueue {
public:
    void push(const TtsMessage& msg);
    void push_high_priority(const TtsMessage& msg);
    std::optional<TtsMessage> pop();
    void clear();
    size_t size() const;
    bool empty() const;

private:
    std::deque<TtsMessage> m_queue;
    mutable std::mutex m_mutex;
};

} // namespace kumo
