#include "kumo/bilibili/api.h"
#include "kumo/bilibili/wbi.h"

// cpp-httplib is a header-only library
// #include <httplib.h>

namespace kumo {

static const char* API_BASE = "https://api.bilibili.com";
static const char* LIVE_BASE = "https://api.live.bilibili.com";

BiliApi::BiliApi()
    : m_inner(std::make_shared<Inner>())
    , m_event_bus(std::make_shared<EventBus>()) {}

void BiliApi::set_cookies(const Cookies& cookies) {
    std::unique_lock lock(m_inner->mutex);
    m_inner->cookies = cookies;
}

void BiliApi::clear_cookies() {
    std::unique_lock lock(m_inner->mutex);
    m_inner->cookies = std::nullopt;
}

bool BiliApi::is_logged_in() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->cookies.has_value() && m_inner->cookies->is_valid();
}

std::optional<RoomId> BiliApi::room_init(uint64_t room_id) {
    std::shared_lock lock(m_inner->mutex);

    // Build URL: https://api.live.bilibili.com/room/v1/Room/room_init?id={room_id}
    // In a real implementation, we'd use cpp-httplib:
    // httplib::Client cli(LIVE_BASE);
    // cli.set_follow_location(true);
    // auto res = cli.Get("/room/v1/Room/room_init?id=" + std::to_string(room_id));
    // ... parse JSON response ...

    // Stub: return default room
    return RoomId{0, room_id, 0};
}

std::optional<RoomInfo> BiliApi::room_info(uint64_t room_id) {
    // GET /room/v1/Room/get_info?room_id={room_id}
    return std::nullopt; // stub
}

std::optional<QrCodeInfo> BiliApi::qr_login() {
    // GET https://api.bilibili.com/x/web-interface/nav
    // Stub
    return std::nullopt;
}

std::optional<QrCodeStatus> BiliApi::qr_login_status(const std::string& qrcode_key) {
    return std::nullopt; // stub
}

std::optional<UserInfoData> BiliApi::user_info() {
    std::shared_lock lock(m_inner->mutex);
    if (!m_inner->cookies) return std::nullopt;
    // GET /x/web-interface/nav with cookies
    return std::nullopt; // stub
}

std::optional<UserInfoData> BiliApi::user_info_by_uid(uint64_t uid) {
    return std::nullopt; // stub
}

std::optional<std::vector<OnlineGoldRankItem>> BiliApi::audience_list(uint64_t room_id, uint32_t page) {
    return std::nullopt; // stub
}

std::optional<std::pair<std::vector<GuardListItem>, uint32_t>> BiliApi::guard_list(uint64_t room_id, uint32_t page) {
    return std::nullopt; // stub
}

std::optional<RtmpInfo> BiliApi::start_live(uint64_t room_id, uint32_t area_v2) {
    return std::nullopt; // stub
}

} // namespace kumo
