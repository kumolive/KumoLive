#include "kumo/messages/messages.h"
#include <random>
#include <chrono>
#include <sstream>
#include <iomanip>

namespace kumo {

// UUID v4 generator
static std::string make_uuid() {
    static thread_local std::mt19937_64 rng(
        std::chrono::steady_clock::now().time_since_epoch().count());
    static thread_local std::uniform_int_distribution<uint64_t> dist64;
    static thread_local std::uniform_int_distribution<uint32_t> dist32;

    uint32_t a = dist32(rng);
    uint32_t b = dist32(rng);
    uint32_t c = dist32(rng) & 0x0FFFFFFF | 0x40000000; // version 4
    uint32_t d = dist32(rng) & 0x3FFFFFFF | 0x80000000; // variant 1

    char buf[37];
    snprintf(buf, sizeof(buf),
        "%08x-%04x-%04x-%04x-%04x%08x",
        a, b >> 16, b & 0xFFFF, c >> 16, c & 0xFFFF, d);
    return buf;
}

// Dynamic emoji map — maps emoji key to WebP URL
static const std::unordered_map<std::string, const char*> DYNAMIC_EMOJI_MAP = {
    {"[轴伊Joi收藏集动态表情包_跑了]", "https://i0.hdslb.com/bfs/garb/3c6551d38c726273798e1cc1e488a3d0633bad5e.webp"},
    {"[轴伊Joi收藏集动态表情包_鞠躬]", "https://i0.hdslb.com/bfs/garb/13fc84be85e70ccd8b12acc8e96e49203e96b1a9.webp"},
    {"[轴伊Joi收藏集动态表情包_摇你]", "https://i0.hdslb.com/bfs/garb/ec27426348fafa3286412d04e8ea851a7a84ce6e.webp"},
    {"[轴伊Joi收藏集动态表情包_愤怒]", "https://i0.hdslb.com/bfs/garb/5fbc1bda6a6d73b83e51cba2119ea65d0dc74689.webp"},
    {"[轴伊Joi收藏集动态表情包_猴]",   "https://i0.hdslb.com/bfs/garb/4c7be23a801094ea00452a2da5ce3184336df331.webp"},
    {"[轴伊Joi收藏集动态表情包_NO]",   "https://i0.hdslb.com/bfs/garb/5d124a80db426608c48c76b27d790c711d39b7b7.webp"},
    {"[轴伊Joi收藏集动态表情包_贴贴]", "https://i0.hdslb.com/bfs/garb/b02b9ebd543b2ee63573daffa191e8a76b7bb384.webp"},
    {"[轴伊Joi收藏集动态表情包_kksk]",  "https://i0.hdslb.com/bfs/garb/518c5b00cdb1329ae8e26d50c52369394d7a2394.webp"},
    {"[轴伊Joi收藏集动态表情包_这辈子完了]", "https://i0.hdslb.com/bfs/garb/85464aa487412c66ab24543a86218fa82fff7f23.webp"},
    {"[轴伊Joi收藏集动态表情包_呆]",   "https://i0.hdslb.com/bfs/garb/09a0bd1deaa02a4255dc886642123dfe2606ddb8.webp"},
    {"[轴伊Joi收藏集动态表情包_唔唔]", "https://i0.hdslb.com/bfs/garb/c82df0783d3cba6af8cebaa43c1f36b1ef3cc0f4.webp"},
    {"[轴伊Joi收藏集动态表情包_啊这]", "https://i0.hdslb.com/bfs/garb/03e899fb3c823cf337b50100718247f25955ee4b.webp"},
    {"[轴伊Joi收藏集动态表情包_失落]", "https://i0.hdslb.com/bfs/garb/dc085a527f49bedf897d08de2bb07fa7c779641b.webp"},
    {"[轴伊Joi收藏集动态表情包_神气]", "https://i0.hdslb.com/bfs/garb/8ed028a87af6c2ef79cdd654f3076d7f7f836514.webp"},
    {"[轴伊Joi收藏集动态表情包_怎么这样]", "https://i0.hdslb.com/bfs/garb/7e239e9ee33a8e168df4bba58cd9a18c40d9af45.webp"},
    {"[轴伊Joi收藏集动态表情包_尼嘻嘻]", "https://i0.hdslb.com/bfs/garb/278ebc1d82a220a127aa60a10b461dbbc71040c6.webp"},
    {"[轴伊Joi收藏集动态表情包_惊]",   "https://i0.hdslb.com/bfs/garb/82f222eebba1160d81244d578dda1c4724276056.webp"},
    {"[轴伊Joi收藏集动态表情包_害怕]", "https://i0.hdslb.com/bfs/garb/4c00a15ab814a8855e3b7106cdc43c1f0a8fa640.webp"},
    {"[轴伊Joi收藏集动态表情包_睡觉]", "https://i0.hdslb.com/bfs/garb/dbec05c6adfd3c56c4fa484b00e4095b4d17cec7.webp"},
    {"[轴伊Joi收藏集动态表情包_爆]",   "https://i0.hdslb.com/bfs/garb/b7e50a289001583cd06eb1149304149ba5ccc78b.webp"},
};

static std::string trim_newlines(const std::string& s) {
    std::string r;
    r.reserve(s.size());
    for (char c : s) {
        if (c != '\r' && c != '\n') r += c;
    }
    return r;
}

// ============================================================================
// DanmuMessage
// ============================================================================

std::optional<DanmuMessage> DanmuMessage::from_raw(const json& body, const MergeUserInfo* user_info) {
    if (!body.contains("info") || !body["info"].is_array()) return std::nullopt;
    const auto& info = body["info"];

    DanmuMessage msg;

    // Side index from merge user info
    msg.side_index = user_info ? static_cast<int32_t>(user_info->index) : -1;

    // Sender info: info[2] = [uid, uname, ...]
    if (info.size() > 2 && info[2].is_array()) {
        const auto& user_arr = info[2];
        if (user_arr.size() > 0 && !user_arr[0].is_null())
            msg.sender.uid = user_arr[0].get<uint64_t>();
        if (user_arr.size() > 1 && user_arr[1].is_string())
            msg.sender.uname = user_arr[1].get<std::string>();
    }

    // Reply info: info[0][15]
    if (info.size() > 0 && info[0].is_array()) {
        const auto& arr0 = info[0];
        if (arr0.size() > 15 && arr0[15].is_object()) {
            const auto& extra = arr0[15];
            if (extra.value("show_reply", false)) {
                if (extra.contains("reply_uname") && extra["reply_uname"].is_string()) {
                    auto reply = extra["reply_uname"].get<std::string>();
                    if (!reply.empty()) msg.reply_uname = reply;
                }
            }
        }
    }

    // Medal info: info[3]
    if (info.size() > 3 && info[3].is_array()) {
        const auto& medal_arr = info[3];
        if (medal_arr.size() >= 12) {
            msg.sender.medal_info.medal_level        = medal_arr[0].get<uint64_t>();
            msg.sender.medal_info.medal_name          = medal_arr[1].get<std::string>();
            msg.sender.medal_info.anchor_uname        = medal_arr[2].get<std::string>();
            msg.sender.medal_info.anchor_roomid       = medal_arr[3].get<uint64_t>();
            msg.sender.medal_info.medal_color         = medal_arr[4].get<uint32_t>();
            msg.sender.medal_info.medal_color_border  = medal_arr[7].get<uint32_t>();
            msg.sender.medal_info.medal_color_start   = medal_arr[8].get<uint32_t>();
            msg.sender.medal_info.medal_color_end     = medal_arr[9].get<uint32_t>();
            msg.sender.medal_info.guard_level         = medal_arr[10].get<uint8_t>();
            msg.sender.medal_info.is_lighted          = medal_arr[11].get<uint64_t>() == 1;
        }
    }

    // Content: info[1]
    if (info.size() > 1 && info[1].is_string()) {
        msg.content = trim_newlines(info[1].get<std::string>());
    } else {
        return std::nullopt;
    }

    // Emoji: info[0][12] == 1, info[0][13] is content
    if (info.size() > 0 && info[0].is_array()) {
        const auto& arr0 = info[0];
        if (arr0.size() > 13 && arr0[12].is_number() && arr0[12].get<int>() == 1) {
            if (!arr0[13].is_null()) {
                EmojiContent emoji;
                emoji.bulge_display   = arr0[13].value("bulge_display", 0);
                emoji.emoticon_unique = arr0[13].value("emoticon_unique", std::string(""));
                emoji.height          = arr0[13].value("height", 0U);
                emoji.in_player_area  = arr0[13].value("in_player_area", 0);
                emoji.is_dynamic      = arr0[13].value("is_dynamic", 0);
                emoji.url             = arr0[13].value("url", std::string(""));
                emoji.width           = arr0[13].value("width", 0U);

                std::string key = emoji.emoticon_unique;
                if (key.size() > 7 && key.substr(0, 7) == "upower_") {
                    key = key.substr(7);
                }
                auto it = DYNAMIC_EMOJI_MAP.find(key);
                if (it != DYNAMIC_EMOJI_MAP.end()) {
                    emoji.url = it->second;
                }
                msg.emoji_content = std::move(emoji);
            }
        }

        // is_generated: info[0][9] > 0
        if (arr0.size() > 9) {
            msg.is_generated = arr0[9].get<int64_t>() > 0;
        }
    }

    // is_special: info[2][2] > 0
    if (info.size() > 2 && info[2].is_array()) {
        const auto& arr2 = info[2];
        if (arr2.size() > 2) {
            msg.is_special = arr2[2].get<int64_t>() > 0;
        }
    }

    return msg;
}

// ============================================================================
// GiftMessage
// ============================================================================

std::optional<GiftMessage> GiftMessage::from_raw(const json& body, uint64_t room_id) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    GiftMessage msg;
    msg.id = make_uuid();
    msg.room = room_id;
    msg.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    // Sender
    msg.sender.uid   = data.value("uid", 0ULL);
    msg.sender.uname = data.value("uname", std::string(""));
    msg.sender.face  = data.value("face", std::string(""));

    // Medal info
    if (data.contains("medal_info") && data["medal_info"].is_object()) {
        msg.sender.medal_info = MedalInfo::from_json(data["medal_info"]);
    }

    // Gift info
    msg.gift_info.id   = data.value("giftId", 0ULL);
    msg.gift_info.name = data.value("giftName", std::string(""));
    msg.gift_info.price = data.value("price", 0ULL);
    msg.gift_info.coin_type = data.value("coin_type", std::string("gold"));

    msg.action = data.value("action", std::string("投喂"));
    msg.num    = static_cast<uint32_t>(data.value("num", 1ULL));
    if (data.contains("timestamp")) msg.timestamp = data["timestamp"].get<int64_t>();

    return msg;
}

// ============================================================================
// GuardMessage
// ============================================================================

std::optional<GuardMessage> GuardMessage::from_raw(const json& body, uint64_t room_id) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    GuardMessage msg;
    msg.id = make_uuid();
    msg.room = room_id;
    msg.timestamp = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();

    msg.sender.uid   = data.value("uid", 0ULL);
    msg.sender.uname = data.value("username", std::string(""));
    msg.sender.face  = data.value("face", std::string(""));

    msg.guard_level = static_cast<uint8_t>(data.value("guard_level", 0ULL));
    msg.num  = static_cast<uint32_t>(data.value("num", 1ULL));
    msg.unit = data.value("unit", std::string("月"));
    msg.price = data.value("price", 0ULL);

    return msg;
}

// ============================================================================
// SuperChatMessage
// ============================================================================

std::optional<SuperChatMessage> SuperChatMessage::from_raw(const json& body, uint64_t room_id) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    if (!data.contains("user_info") || !data.contains("id") || !data.contains("uid"))
        return std::nullopt;

    const auto& user_info = data["user_info"];

    SuperChatMessage msg;
    msg.id   = std::to_string(data["id"].get<uint64_t>());
    msg.room = room_id;

    msg.sender.uid   = data["uid"].get<uint64_t>();
    msg.sender.uname = user_info.value("uname", std::string(""));
    msg.sender.face  = user_info.value("face", std::string(""));

    // Medal info
    if (data.contains("medal_info") && data["medal_info"].is_object()) {
        msg.sender.medal_info = MedalInfo::from_json(data["medal_info"]);
    }

    msg.message  = data.value("message", std::string(""));
    msg.price    = data.value("price", 0ULL);
    msg.start_time = data.value("start_time", 0LL);
    msg.end_time   = data.value("end_time", 0LL);
    msg.timestamp  = msg.start_time;

    msg.background_color        = data.value("background_color", std::string("#EDF5FF"));
    msg.background_bottom_color = data.value("background_bottom_color", std::string("#2A60B2"));

    return msg;
}

// ============================================================================
// InteractMessage
// ============================================================================

std::optional<InteractMessage> InteractMessage::from_raw(const json& body) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    InteractMessage msg;
    msg.sender.uid   = data.value("uid", 0ULL);
    msg.sender.uname = data.value("uname", std::string(""));

    // Medal info
    if (data.contains("fans_medal") && data["fans_medal"].is_object()) {
        msg.sender.medal_info = MedalInfo::from_json(data["fans_medal"]);
    }

    msg.action = static_cast<int32_t>(data.value("msg_type", 0LL));
    return msg;
}

// ============================================================================
// EntryEffectMessage
// ============================================================================

std::optional<EntryEffectMessage> EntryEffectMessage::from_raw(const json& body) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    EntryEffectMessage msg;
    msg.sender.uid = data.value("uid", 0ULL);

    // Extract uname from copy_writing: "<%{uname}%> ..."
    std::string copy_writing = data.value("copy_writing", std::string(""));
    auto start = copy_writing.find("<%");
    if (start != std::string::npos) {
        auto end = copy_writing.find("%>", start + 2);
        if (end != std::string::npos) {
            msg.sender.uname = copy_writing.substr(start + 2, end - start - 2);
        }
    }

    bool is_guard = copy_writing.find("舰长") != std::string::npos ||
                    copy_writing.find("提督") != std::string::npos ||
                    copy_writing.find("总督") != std::string::npos;

    msg.privilege_type = is_guard
        ? static_cast<uint8_t>(data.value("privilege_type", 0ULL))
        : 0;

    return msg;
}

// ============================================================================
// RoomChangeMessage
// ============================================================================

std::optional<RoomChangeMessage> RoomChangeMessage::from_raw(const json& body) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];

    RoomChangeMessage msg;
    msg.title            = data.value("title", std::string(""));
    msg.area_name        = data.value("area_name", std::string(""));
    msg.parent_area_name = data.value("parent_area_name", std::string(""));
    return msg;
}

// ============================================================================
// OnlineRankCountMessage
// ============================================================================

std::optional<OnlineRankCountMessage> OnlineRankCountMessage::from_raw(const json& body) {
    if (!body.contains("data") || !body["data"].is_object()) return std::nullopt;
    const auto& data = body["data"];
    if (!data.contains("count")) return std::nullopt;

    OnlineRankCountMessage msg;
    msg.count = data["count"].get<uint64_t>();
    return msg;
}

// ============================================================================
// WarningMessage
// ============================================================================

std::optional<WarningMessage> WarningMessage::from_raw(const json& body) {
    WarningMessage msg;
    msg.msg = body.value("msg", std::string(""));
    return msg;
}

// ============================================================================
// CutOffMessage
// ============================================================================

std::optional<CutOffMessage> CutOffMessage::from_raw(const json& body) {
    CutOffMessage msg;
    msg.msg = body.value("msg", std::string(""));
    return msg;
}

} // namespace kumo
