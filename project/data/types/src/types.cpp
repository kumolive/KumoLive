#include "kumo/types/types.h"

namespace kumo {

// ============================================================================
// Cookies
// ============================================================================

Cookies Cookies::from_query_string(const std::string& query) {
    Cookies cookies;
    std::string key, value;
    bool in_key = true;

    for (size_t i = 0; i <= query.size(); ++i) {
        char c = (i < query.size()) ? query[i] : '&';
        if (c == '=' && in_key) {
            in_key = false;
        } else if (c == '&') {
            if (!key.empty()) {
                if (key == "DedeUserID")       cookies.dede_user_id = value;
                else if (key == "DedeUserID__ckMd5") cookies.dede_user_id_ck_md5 = value;
                else if (key == "Expires")      cookies.expires = value;
                else if (key == "SESSDATA")     cookies.sessdata = value;
                else if (key == "bili_jct")     cookies.bili_jct = value;
                else if (key == "gourl")        cookies.gourl = value;
            }
            key.clear();
            value.clear();
            in_key = true;
        } else if (in_key) {
            key += c;
        } else {
            value += c;
        }
    }
    return cookies;
}

std::string Cookies::to_cookie_string() const {
    if (sessdata.empty()) return "";
    std::string encoded = sessdata;
    // Simple URL-encode: only % needs encoding in SESSDATA context
    // The Rust code uses urlencoding::encode for SESSDATA
    return "SESSDATA=" + encoded +
           "; DedeUserID=" + dede_user_id +
           "; DedeUserID_ckMd5=" + dede_user_id_ck_md5 +
           "; bili_jct=" + bili_jct +
           "; Expires=" + expires;
}

bool Cookies::is_valid() const {
    return !sessdata.empty() && !bili_jct.empty();
}

std::optional<uint64_t> Cookies::user_id() const {
    try {
        return std::stoull(dede_user_id);
    } catch (...) {
        return std::nullopt;
    }
}

// ============================================================================
// RoomId
// ============================================================================

RoomId RoomId::create(uint64_t short_id, uint64_t room_id, uint64_t owner_uid) {
    return {short_id, room_id, owner_uid};
}

RoomId RoomId::default_room() {
    return {0, 21484828, 61639371};
}

bool RoomId::matches(uint64_t rid) const {
    return short_id == rid || room_id == rid;
}

uint64_t RoomId::display_id() const {
    return short_id != 0 ? short_id : room_id;
}

uint64_t RoomId::real_id() const { return room_id; }
uint64_t RoomId::owner() const { return owner_uid; }

// ============================================================================
// MedalInfo
// ============================================================================

std::string MedalInfo::color_to_hex(uint32_t color) {
    char buf[8];
    snprintf(buf, sizeof(buf), "#%06x", color);
    return buf;
}

std::string MedalInfo::medal_color_hex() const { return color_to_hex(medal_color); }
std::string MedalInfo::medal_color_border_hex() const { return color_to_hex(medal_color_border); }

MedalInfo MedalInfo::from_json(const json& j) {
    MedalInfo m;
    m.medal_level       = j.value("medal_level", 0);
    m.medal_name        = j.value("medal_name", "");
    m.anchor_uname      = j.value("anchor_uname", "");
    m.anchor_roomid     = j.value("anchor_roomid", 0ULL);
    m.guard_level       = j.value("guard_level", 0);
    m.medal_color       = j.value("medal_color", 0U);
    m.medal_color_border= j.value("medal_color_border", 0U);
    m.medal_color_start = j.value("medal_color_start", 0U);
    m.medal_color_end   = j.value("medal_color_end", 0U);
    m.is_lighted        = j.value("is_lighted", false);
    return m;
}

// ============================================================================
// Sender
// ============================================================================

Sender Sender::from_json(const json& j) {
    Sender s;
    s.uid   = j.value("uid", 0ULL);
    s.uname = j.value("uname", "");
    s.face  = j.value("face", "");
    if (j.contains("medal_info")) {
        s.medal_info = MedalInfo::from_json(j["medal_info"]);
    }
    return s;
}

// ============================================================================
// Helper functions
// ============================================================================

const char* guard_level_name(uint8_t level) {
    switch (level) {
        case 1: return "总督";
        case 2: return "提督";
        case 3: return "舰长";
        default: return "";
    }
}

const char* guard_icon_url(uint8_t level) {
    switch (level) {
        case 1: return "https://i0.hdslb.com/bfs/live/143f5ec3003b4080d1b5f817a9efdca46d631945.png@44w_44h.webp";
        case 2: return "https://i0.hdslb.com/bfs/live/98a201c14a64e860a758f089144dcf3f42e7038c.png@44w_44h.webp";
        case 3: return "https://i0.hdslb.com/bfs/live/334f567013c5a9e7b93c04a0e80e31dd2af07b52.png@44w_44h.webp";
        default: return "";
    }
}

} // namespace kumo
