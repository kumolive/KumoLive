#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace kumo {

// ============================================================================
// Cookies
// ============================================================================

struct Cookies {
    std::string dede_user_id;
    std::string dede_user_id_ck_md5;
    std::string expires;
    std::string sessdata;
    std::string bili_jct;
    std::string gourl;

    static Cookies from_query_string(const std::string& query);
    std::string to_cookie_string() const;
    bool is_valid() const;
    std::optional<uint64_t> user_id() const;
};

// ============================================================================
// RoomId
// ============================================================================

struct RoomId {
    uint64_t short_id = 0;
    uint64_t room_id = 0;
    uint64_t owner_uid = 0;

    static RoomId create(uint64_t short_id, uint64_t room_id, uint64_t owner_uid);
    static RoomId default_room();

    bool matches(uint64_t rid) const;
    uint64_t display_id() const;
    uint64_t real_id() const;
    uint64_t owner() const;
};

// ============================================================================
// MedalInfo
// ============================================================================

struct MedalInfo {
    uint64_t anchor_roomid = 0;
    std::string anchor_uname;
    uint8_t guard_level = 0;
    uint32_t medal_color = 0;
    uint32_t medal_color_border = 0;
    uint32_t medal_color_start = 0;
    uint32_t medal_color_end = 0;
    uint8_t medal_level = 0;
    std::string medal_name;
    bool is_lighted = false;

    static std::string color_to_hex(uint32_t color);
    std::string medal_color_hex() const;
    std::string medal_color_border_hex() const;

    static MedalInfo from_json(const json& j);
};

// ============================================================================
// Sender
// ============================================================================

struct Sender {
    uint64_t uid = 0;
    std::string uname;
    std::string face;
    MedalInfo medal_info;

    static Sender from_json(const json& j);
};

// ============================================================================
// EmojiContent
// ============================================================================

struct EmojiContent {
    int32_t bulge_display = 0;
    std::string emoticon_unique;
    uint32_t height = 0;
    int32_t in_player_area = 0;
    int32_t is_dynamic = 0;
    std::string url;
    uint32_t width = 0;
};

// ============================================================================
// MergeUserInfo
// ============================================================================

struct MergeUserInfo {
    size_t index = 0;
    std::string uid;
    std::string name;
};

// ============================================================================
// WindowType
// ============================================================================

enum class WindowType : uint8_t {
    Invalid = 0,
    Main,
    Gift,
    SuperChat,
    Setting,
    Detail,
    Rank,
};

static const char* window_type_name(WindowType t) {
    switch (t) {
        case WindowType::Invalid:    return "invalid";
        case WindowType::Main:       return "main";
        case WindowType::Gift:       return "gift";
        case WindowType::SuperChat:  return "superchat";
        case WindowType::Setting:    return "setting";
        case WindowType::Detail:     return "detail";
        case WindowType::Rank:       return "rank";
        default:                     return "invalid";
    }
}

static WindowType window_type_from_name(const std::string& name) {
    if (name == "main")       return WindowType::Main;
    if (name == "gift")       return WindowType::Gift;
    if (name == "superchat")  return WindowType::SuperChat;
    if (name == "setting")    return WindowType::Setting;
    if (name == "detail")     return WindowType::Detail;
    if (name == "rank")       return WindowType::Rank;
    return WindowType::Invalid;
}

// ============================================================================
// RecordType
// ============================================================================

enum class RecordType : uint8_t {
    Danmu = 0,
    Gift = 1,
    SuperChat = 2,
    Guard = 3,
    Interact = 4,
    EntryEffect = 5,
};

// ============================================================================
// DanmuRecord
// ============================================================================

struct DanmuRecord {
    RecordType record_type = RecordType::Danmu;
    std::string content;
    int64_t timestamp = 0;
};

// ============================================================================
// DetailInfo
// ============================================================================

struct DetailInfo {
    Sender sender;
    std::vector<DanmuRecord> danmus;
};

// ============================================================================
// InteractAction
// ============================================================================

enum class InteractAction : int32_t {
    Enter = 1,
    Follow = 2,
    Share = 3,
    SpecialFollow = 4,
    MutualFollow = 5,
};

static std::optional<InteractAction> interact_action_from_i32(int32_t value) {
    switch (value) {
        case 1:  return InteractAction::Enter;
        case 2:  return InteractAction::Follow;
        case 3:  return InteractAction::Share;
        case 4:  return InteractAction::SpecialFollow;
        case 5:  return InteractAction::MutualFollow;
        default: return std::nullopt;
    }
}

static const char* interact_action_desc(InteractAction a) {
    switch (a) {
        case InteractAction::Enter:         return "进入直播间";
        case InteractAction::Follow:        return "关注了直播间";
        case InteractAction::Share:         return "分享了直播间";
        case InteractAction::SpecialFollow: return "特别关注了直播间";
        case InteractAction::MutualFollow:  return "与主播互粉了";
        default:                            return "";
    }
}

// ============================================================================
// API response types (shared by events and bilibili/api)
// ============================================================================

struct UserInfoData {
    std::string uname;
    std::string face;
    uint64_t uid = 0;
    uint64_t money = 0;
};

struct QrCodeStatus {
    bool success = false;
    int32_t status = 0;
    std::string message;
    std::optional<Cookies> cookies;
};

struct OnlineGoldRankItem {
    uint64_t uid = 0;
    std::string uname;
    std::string face;
    uint64_t score = 0;
};

struct GuardListItem {
    uint64_t uid = 0;
    std::string username;
    uint8_t guard_level = 0;
};

// ============================================================================
// Helper functions
// ============================================================================

const char* guard_level_name(uint8_t level);
const char* guard_icon_url(uint8_t level);

} // namespace kumo
