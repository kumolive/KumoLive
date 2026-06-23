#pragma once

#include "kumo/events/events.h"
#include "kumo/types/types.h"
#include <string>
#include <memory>
#include <shared_mutex>
#include <thread>
#include <atomic>
#include <functional>

namespace kumo {

// ============================================================================
// BiliWebSocket — WebSocket danmaku connection
// ============================================================================

class BiliWebSocket {
public:
    BiliWebSocket(std::shared_ptr<EventBus> event_bus);
    ~BiliWebSocket();

    BiliWebSocket(const BiliWebSocket&) = delete;
    BiliWebSocket& operator=(const BiliWebSocket&) = delete;

    bool connect(uint64_t room_id, const std::string& server = "broadcastlv.chat.bilibili.com");
    void disconnect();
    bool is_connected() const;

    void set_merge_user_info(const std::vector<MergeUserInfo>& info);

private:
    struct Inner {
        std::shared_ptr<EventBus> event_bus;
        std::atomic<bool> connected{false};
        std::atomic<bool> should_stop{false};
        std::thread worker;
        uint64_t room_id = 0;
        std::shared_mutex mutex;
        std::vector<MergeUserInfo> merge_users;
    };
    std::shared_ptr<Inner> m_inner;

    void worker_loop(const std::string& server);
    void dispatch_message(const json& body);
};

} // namespace kumo
