#pragma once

#include "kumo/types/types.h"
#include <unordered_map>
#include <string>
#include <optional>

namespace kumo {

// ============================================================================
// DanmuMessage
// ============================================================================

struct DanmuMessage {
    Sender sender;
    std::string content;
    bool is_generated = false;
    bool is_special = false;
    bool is_mirror = false;
    std::optional<EmojiContent> emoji_content;
    int32_t side_index = -1;
    std::optional<std::string> reply_uname;

    static std::optional<DanmuMessage> from_raw(const json& body, const MergeUserInfo* user_info = nullptr);
};

// ============================================================================
// GiftInfo
// ============================================================================

struct GiftInfo {
    uint64_t id = 0;
    std::string name;
    uint64_t price = 0;
    std::string coin_type;
    std::string img_basic;
    std::string img_dynamic;
    std::string gif;
    std::string webp;
};

// ============================================================================
// GiftMessage
// ============================================================================

struct GiftMessage {
    std::string id;
    uint64_t room = 0;
    GiftInfo gift_info;
    Sender sender;
    std::string action;
    uint32_t num = 0;
    int64_t timestamp = 0;
    bool archived = false;

    static std::optional<GiftMessage> from_raw(const json& body, uint64_t room_id);
};

// ============================================================================
// GuardMessage
// ============================================================================

struct GuardMessage {
    std::string id;
    uint64_t room = 0;
    Sender sender;
    uint32_t num = 0;
    std::string unit;
    uint8_t guard_level = 0;
    uint64_t price = 0;
    int64_t timestamp = 0;
    bool archived = false;

    static std::optional<GuardMessage> from_raw(const json& body, uint64_t room_id);
};

// ============================================================================
// SuperChatMessage
// ============================================================================

struct SuperChatMessage {
    std::string id;
    uint64_t room = 0;
    Sender sender;
    std::string message;
    uint64_t price = 0;
    int64_t timestamp = 0;
    int64_t start_time = 0;
    int64_t end_time = 0;
    std::string background_color;
    std::string background_bottom_color;
    bool archived = false;

    static std::optional<SuperChatMessage> from_raw(const json& body, uint64_t room_id);
};

// ============================================================================
// InteractMessage
// ============================================================================

struct InteractMessage {
    Sender sender;
    int32_t action = 0;

    static std::optional<InteractMessage> from_raw(const json& body);
};

// ============================================================================
// EntryEffectMessage
// ============================================================================

struct EntryEffectMessage {
    Sender sender;
    uint8_t privilege_type = 0;

    static std::optional<EntryEffectMessage> from_raw(const json& body);
};

// ============================================================================
// RoomChangeMessage
// ============================================================================

struct RoomChangeMessage {
    std::string title;
    std::string area_name;
    std::string parent_area_name;

    static std::optional<RoomChangeMessage> from_raw(const json& body);
};

// ============================================================================
// OnlineRankCountMessage
// ============================================================================

struct OnlineRankCountMessage {
    uint64_t count = 0;

    static std::optional<OnlineRankCountMessage> from_raw(const json& body);
};

// ============================================================================
// WarningMessage
// ============================================================================

struct WarningMessage {
    std::string msg;

    static std::optional<WarningMessage> from_raw(const json& body);
};

// ============================================================================
// CutOffMessage
// ============================================================================

struct CutOffMessage {
    std::string msg;

    static std::optional<CutOffMessage> from_raw(const json& body);
};

} // namespace kumo
