#pragma once

#include "kumo/types/types.h"
#include "kumo/events/events.h"
#include "kumo/bilibili/wbi.h"
#include <optional>
#include <vector>
#include <string>
#include <memory>
#include <shared_mutex>

namespace kumo {

struct RoomInfo {
    RoomId room_id;
    std::string title;
    uint8_t live_status = 0;
    uint64_t area_id = 0;
    uint64_t online = 0;
    std::string cover;
    std::string uid;
};

struct QrCodeInfo {
    std::string url;
    std::string qrcode_key;
};

struct RtmpInfo {
    std::string addr;
    std::string code;
};

// ============================================================================
// BiliApi — HTTP API client for Bilibili
// ============================================================================

class BiliApi {
public:
    BiliApi();

    void set_cookies(const Cookies& cookies);
    void clear_cookies();
    bool is_logged_in() const;

    // Room
    std::optional<RoomId> room_init(uint64_t room_id);
    std::optional<RoomInfo> room_info(uint64_t room_id);

    // Login
    std::optional<QrCodeInfo> qr_login();
    std::optional<QrCodeStatus> qr_login_status(const std::string& qrcode_key);

    // User
    std::optional<UserInfoData> user_info();
    std::optional<UserInfoData> user_info_by_uid(uint64_t uid);

    // Audience
    std::optional<std::vector<OnlineGoldRankItem>> audience_list(uint64_t room_id, uint32_t page = 1);

    // Guard
    std::optional<std::pair<std::vector<GuardListItem>, uint32_t>> guard_list(uint64_t room_id, uint32_t page = 1);

    // Start live
    std::optional<RtmpInfo> start_live(uint64_t room_id, uint32_t area_v2 = 0);

    std::shared_ptr<EventBus> event_bus() { return m_event_bus; }

private:
    struct Inner {
        std::optional<Cookies> cookies;
        WbiSigner wbi;
        std::shared_mutex mutex;
    };
    std::shared_ptr<Inner> m_inner;
    std::shared_ptr<EventBus> m_event_bus;

    void emit(Event e) { m_event_bus->emit(e); }
};

} // namespace kumo
