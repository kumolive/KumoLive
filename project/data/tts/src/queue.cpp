#include "kumo/tts/queue.h"
#include <algorithm>

namespace kumo {

void TtsQueue::push(const TtsMessage& msg) {
    std::unique_lock lock(m_mutex);
    m_queue.push_back(msg);
}

void TtsQueue::push_high_priority(const TtsMessage& msg) {
    std::unique_lock lock(m_mutex);
    // Insert before the first item with lower or equal priority
    auto it = std::find_if(m_queue.begin(), m_queue.end(),
        [&msg](const TtsMessage& m) { return m.priority > msg.priority; });
    m_queue.insert(it, msg);
}

std::optional<TtsMessage> TtsQueue::pop() {
    std::unique_lock lock(m_mutex);
    if (m_queue.empty()) return std::nullopt;
    auto msg = m_queue.front();
    m_queue.pop_front();
    return msg;
}

void TtsQueue::clear() {
    std::unique_lock lock(m_mutex);
    m_queue.clear();
}

size_t TtsQueue::size() const {
    std::unique_lock lock(m_mutex);
    return m_queue.size();
}

bool TtsQueue::empty() const {
    std::unique_lock lock(m_mutex);
    return m_queue.empty();
}

} // namespace kumo
