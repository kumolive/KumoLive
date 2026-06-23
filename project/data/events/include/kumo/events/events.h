#pragma once

#include "kumo/messages/messages.h"
#include <variant>
#include <functional>
#include <unordered_map>
#include <vector>
#include <shared_mutex>
#include <memory>
#include <condition_variable>
#include <queue>
#include <chrono>

namespace kumo {

// ============================================================================
// Event — all event types as std::variant
// ============================================================================

struct UpdateRoomEvent     { RoomId room_id; std::string title; uint8_t live_status; uint64_t area_id; };
struct UpdateOnlineEvent   { uint64_t count = 0; };
struct ConfigChangedEvent  { std::string key; json value; };
struct ConfigLoadedEvent   {
    bool always_on_top, guard_effect, level_effect, lite_mode, medal_display, interact_display;
    float opacity;
    std::string theme;
    float font_size;
    bool tts_enabled, tts_gift_enabled, tts_sc_enabled;
    float tts_volume;
    size_t max_danmu_count;
    std::string log_level;
    bool auto_update_check;
};
struct DetailUpdateEvent   { DetailInfo info; };
struct ConnectionStatusEvent { bool connected = false; };
struct LoginStatusChangedEvent { bool logged_in = false; std::optional<UserInfoData> user_info; };
struct QrCodeGeneratedEvent { std::string url; std::string qrcode_key; };
struct QrLoginStatusEvent  { QrCodeStatus status; };
struct RtmpInfoEvent       { std::string addr; std::string code; };
struct FaceAuthRequiredEvent { std::string qr_url; };
struct UserInfoFetchedEvent { uint64_t uid = 0; UserInfoData user_info; };
struct AudienceListFetchedEvent { std::vector<OnlineGoldRankItem> list; };
struct GuardListFetchedEvent { std::vector<GuardListItem> list; uint32_t total = 0; uint32_t page = 0; };
struct UpdateCheckResultEvent {
    bool has_update = false;
    std::string current_version;
    std::string latest_version;
    std::string release_url;
    std::optional<std::string> error;
};

struct LiveStartEvent {};
struct LiveEndEvent {};
struct RequestQrLoginEvent {};
struct RequestLogoutEvent {};
struct ClearDanmuListEvent {};
struct DataClearedEvent {};

using Event = std::variant<
    UpdateRoomEvent,
    UpdateOnlineEvent,
    DanmuMessage,
    GiftMessage,
    GuardMessage,
    SuperChatMessage,
    InteractMessage,
    EntryEffectMessage,
    RoomChangeMessage,
    ConfigChangedEvent,
    ConfigLoadedEvent,
    DetailUpdateEvent,
    ConnectionStatusEvent,
    LoginStatusChangedEvent,
    QrCodeGeneratedEvent,
    QrLoginStatusEvent,
    RtmpInfoEvent,
    FaceAuthRequiredEvent,
    UserInfoFetchedEvent,
    AudienceListFetchedEvent,
    GuardListFetchedEvent,
    UpdateCheckResultEvent,
    WarningMessage,
    CutOffMessage,
    LiveStartEvent,
    LiveEndEvent,
    RequestQrLoginEvent,
    RequestLogoutEvent,
    ClearDanmuListEvent,
    DataClearedEvent
>;

const char* event_type_name(const Event& event);

// ============================================================================
// BroadcastChannel — thread-safe multi-producer multi-consumer channel
// ============================================================================

template<typename T>
class BroadcastChannel {
public:
    struct Receiver {
        std::queue<T> queue;
        std::shared_mutex mutex;
        bool active = true;
    };

    BroadcastChannel(size_t capacity = 1000) : m_capacity(capacity) {}

    void send(const T& value) {
        std::unique_lock lock(m_mutex);
        for (auto& recv : m_receivers) {
            std::unique_lock rlock(recv->mutex);
            if (recv->active) {
                if (recv->queue.size() >= m_capacity)
                    recv->queue.pop();
                recv->queue.push(value);
            }
        }
        m_cv.notify_all();
    }

    std::shared_ptr<Receiver> subscribe() {
        std::unique_lock lock(m_mutex);
        auto recv = std::make_shared<Receiver>();
        m_receivers.push_back(recv);
        return recv;
    }

    void unsubscribe(std::shared_ptr<Receiver>& recv) {
        std::unique_lock lock(m_mutex);
        std::unique_lock rlock(recv->mutex);
        recv->active = false;
        m_receivers.erase(
            std::remove(m_receivers.begin(), m_receivers.end(), recv),
            m_receivers.end());
    }

    // Block until a value is available, then pop
    bool recv(std::shared_ptr<Receiver>& recv, T& out, std::chrono::milliseconds timeout = std::chrono::milliseconds(100)) {
        std::unique_lock lock(recv->mutex);
        m_cv.wait_for(lock, timeout, [&] { return !recv->queue.empty() || !recv->active; });
        if (!recv->active || recv->queue.empty()) return false;
        out = std::move(recv->queue.front());
        recv->queue.pop();
        return true;
    }

private:
    size_t m_capacity;
    std::shared_mutex m_mutex;
    std::condition_variable_any m_cv;
    std::vector<std::shared_ptr<Receiver>> m_receivers;
};

// ============================================================================
// EventBus
// ============================================================================

class EventBus {
public:
    using Handler = std::function<void(const Event&)>;

    EventBus();
    ~EventBus() = default;

    void emit(const Event& event);

    void on(const std::string& event_type, Handler handler);
    void on_all(Handler handler);

    std::shared_ptr<BroadcastChannel<Event>::Receiver> subscribe();

private:
    struct Inner {
        BroadcastChannel<Event> sender;
        std::shared_mutex handlers_mutex;
        std::unordered_map<std::string, std::vector<Handler>> handlers;
    };
    std::shared_ptr<Inner> m_inner;
};

// ============================================================================
// EventType enum
// ============================================================================

enum class EventType {
    UpdateRoom,
    UpdateOnline,
    NewDanmu,
    NewGift,
    NewGuard,
    NewSuperChat,
    NewInteract,
    NewEntryEffect,
    RoomChange,
    ConfigChanged,
    DetailUpdate,
    LiveStart,
    LiveEnd,
    ConnectionStatus,
    LoginStatusChanged,
    RequestQrLogin,
    QrCodeGenerated,
    QrLoginStatus,
    RequestLogout,
    ClearDanmuList,
    UserInfoFetched,
    AudienceListFetched,
    GuardListFetched,
};

const char* event_type_str(EventType type);

} // namespace kumo
