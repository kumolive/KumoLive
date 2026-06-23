#include "kumo/config/config.h"
#include <fstream>
#include <sstream>

namespace kumo {

// ============================================================================
// Base64 encode/decode
// ============================================================================

static const char BASE64_CHARS[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static std::string base64_encode(const std::string& input) {
    std::string out;
    out.reserve(((input.size() + 2) / 3) * 4);
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            out.push_back(BASE64_CHARS[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6)
        out.push_back(BASE64_CHARS[((val << 8) >> (valb + 8)) & 0x3F]);
    while (out.size() % 4)
        out.push_back('=');
    return out;
}

static std::optional<std::string> base64_decode(const std::string& input) {
    static int8_t T[256] = {};
    static bool init = false;
    if (!init) {
        for (int i = 0; i < 256; ++i) T[i] = -1;
        for (int i = 0; i < 64; ++i) T[(unsigned char)BASE64_CHARS[i]] = i;
        init = true;
    }

    std::string out;
    int val = 0, valb = -8;
    for (unsigned char c : input) {
        if (c == '=') break;
        int v = T[c];
        if (v == -1) return std::nullopt;
        val = (val << 6) + v;
        valb += 6;
        if (valb >= 0) {
            out.push_back(static_cast<char>((val >> valb) & 0xFF));
            valb -= 8;
        }
    }
    return out;
}

// ============================================================================
// JSON serialization for Cookies
// ============================================================================

static void to_json(json& j, const Cookies& c) {
    j["DedeUserID"] = c.dede_user_id;
    j["DedeUserID__ckMd5"] = c.dede_user_id_ck_md5;
    j["Expires"] = c.expires;
    j["SESSDATA"] = c.sessdata;
    j["bili_jct"] = c.bili_jct;
    j["gourl"] = c.gourl;
}

static void from_json(const json& j, Cookies& c) {
    j.at("DedeUserID").get_to(c.dede_user_id);
    j.at("DedeUserID__ckMd5").get_to(c.dede_user_id_ck_md5);
    j.at("Expires").get_to(c.expires);
    j.at("SESSDATA").get_to(c.sessdata);
    j.at("bili_jct").get_to(c.bili_jct);
    c.gourl = j.value("gourl", "");
}

// Cookies base64 encoded serialization
static void to_json_cookies(json& j, const std::optional<Cookies>& cookies) {
    if (cookies) {
        json cj;
        to_json(cj, *cookies);
        j = base64_encode(cj.dump());
    }
}

static void from_json_cookies(const json& j, std::optional<Cookies>& cookies) {
    if (j.is_null()) {
        cookies = std::nullopt;
    } else if (j.is_string()) {
        auto decoded = base64_decode(j.get<std::string>());
        if (decoded) {
            try {
                auto cj = json::parse(*decoded);
                Cookies c;
                from_json(cj, c);
                cookies = c;
            } catch (...) {
                cookies = std::nullopt;
            }
        }
    } else if (j.is_object()) {
        // Old format: cookies stored as plain JSON object
        try {
            Cookies c;
            from_json(j, c);
            cookies = c;
        } catch (...) {
            cookies = std::nullopt;
        }
    }
}

// ============================================================================
// JSON serialization for RoomId
// ============================================================================

static void to_json(json& j, const RoomId& r) {
    j["short_id"] = r.short_id;
    j["room_id"] = r.room_id;
    j["owner_uid"] = r.owner_uid;
}

static void from_json(const json& j, RoomId& r) {
    j.at("short_id").get_to(r.short_id);
    j.at("room_id").get_to(r.room_id);
    j.at("owner_uid").get_to(r.owner_uid);
}

// ============================================================================
// JSON serialization for Config
// ============================================================================

void to_json(json& j, const Config& c) {
    j["version"] = c.version;
    to_json_cookies(j["cookies"], c.cookies);
    if (c.room) {
        json rj;
        to_json(rj, *c.room);
        j["room"] = rj;
    } else {
        j["room"] = nullptr;
    }
    j["login"] = c.login;
    j["merge"] = c.merge;
    j["merge_rooms"] = c.merge_rooms;
    j["always_on_top"] = c.always_on_top;
    j["max_detail_entry"] = c.max_detail_entry;
    j["guard_effect"] = c.guard_effect;
    j["level_effect"] = c.level_effect;
    j["opacity"] = c.opacity;
    j["lite_mode"] = c.lite_mode;
    j["medal_display"] = c.medal_display;
    j["interact_display"] = c.interact_display;
    j["theme"] = c.theme;
    j["font_size"] = c.font_size;
    for (const auto& [k, v] : c.windows) {
        json wj;
        to_json(wj, v);
        j["windows"][k] = wj;
    }
    j["log_level"] = c.log_level;
    j["max_danmu_count"] = c.max_danmu_count;
    j["tts_provider"] = static_cast<int>(c.tts_provider);
    j["tts_aliyun_app_key"] = c.tts_aliyun_app_key;
    j["tts_aliyun_access_key_id"] = c.tts_aliyun_access_key_id;
    j["tts_aliyun_access_key_secret"] = c.tts_aliyun_access_key_secret;
    j["tts_custom_url"] = c.tts_custom_url;
    j["tts_enabled"] = c.tts_enabled;
    j["tts_gift_enabled"] = c.tts_gift_enabled;
    j["tts_sc_enabled"] = c.tts_sc_enabled;
    j["tts_volume"] = c.tts_volume;
    j["auto_update_check"] = c.auto_update_check;
    for (const auto& [k, v] : c.extra.items())
        j[k] = v;
}

void from_json(const json& j, Config& c) {
    c.version = j.value("version", 2U);
    from_json_cookies(j.value("cookies", json(nullptr)), c.cookies);
    auto room_j = j.value("room", json(nullptr));
    if (room_j.is_object()) {
        RoomId r;
        from_json(room_j, r);
        c.room = r;
    }
    c.login = j.value("login", false);
    c.merge = j.value("merge", false);
    if (j.contains("merge_rooms")) c.merge_rooms = j["merge_rooms"].get<std::vector<RoomId>>();
    c.always_on_top = j.value("always_on_top", false);
    c.max_detail_entry = j.value("max_detail_entry", 100UL);
    c.guard_effect = j.value("guard_effect", false);
    c.level_effect = j.value("level_effect", false);
    c.opacity = j.value("opacity", 1.0f);
    c.lite_mode = j.value("lite_mode", false);
    c.medal_display = j.value("medal_display", true);
    c.interact_display = j.value("interact_display", false);
    c.theme = j.value("theme", std::string("dark"));
    c.font_size = j.value("font_size", 14.0f);
    if (j.contains("windows")) {
        for (const auto& [k, v] : j["windows"].items())
            c.windows[k] = v.get<WindowConfig>();
    }
    c.log_level = j.value("log_level", std::string("info"));
    c.max_danmu_count = j.value("max_danmu_count", 200UL);
    c.tts_provider = static_cast<TtsProvider>(j.value("tts_provider", 0));
    c.tts_aliyun_app_key = j.value("tts_aliyun_app_key", std::string(""));
    c.tts_aliyun_access_key_id = j.value("tts_aliyun_access_key_id", std::string(""));
    c.tts_aliyun_access_key_secret = j.value("tts_aliyun_access_key_secret", std::string(""));
    c.tts_custom_url = j.value("tts_custom_url", std::string(""));
    c.tts_enabled = j.value("tts_enabled", false);
    c.tts_gift_enabled = j.value("tts_gift_enabled", false);
    c.tts_sc_enabled = j.value("tts_sc_enabled", false);
    c.tts_volume = j.value("tts_volume", 1.0f);
    c.auto_update_check = j.value("auto_update_check", true);
    c.extra.clear();
    for (const auto& [k, v] : j.items()) {
        // Only add unknown fields as extra
        static const std::string known[] = {
            "version","cookies","room","login","merge","merge_rooms","always_on_top",
            "max_detail_entry","guard_effect","level_effect","opacity","lite_mode",
            "medal_display","interact_display","theme","font_size","windows","log_level",
            "max_danmu_count","tts_provider","tts_aliyun_app_key","tts_aliyun_access_key_id",
            "tts_aliyun_access_key_secret","tts_custom_url","tts_enabled","tts_gift_enabled",
            "tts_sc_enabled","tts_volume","auto_update_check"
        };
        bool is_known = false;
        for (const auto& kn : known) {
            if (k == kn) { is_known = true; break; }
        }
        if (!is_known) c.extra[k] = v;
    }
}

// ============================================================================
// ConfigStore
// ============================================================================

ConfigStore::ConfigStore() : m_inner(std::make_shared<Inner>()) {
    init_default_path();
}

ConfigStore::ConfigStore(const std::filesystem::path& path) : m_inner(std::make_shared<Inner>()) {
    m_inner->config_path = path;
    if (std::filesystem::exists(path)) {
        try {
            std::ifstream file(path);
            json j;
            file >> j;
            m_inner->config = j.get<Config>();
        } catch (...) {
            m_inner->config = Config{};
        }
    } else {
        m_inner->config = Config{};
    }
}

void ConfigStore::init_default_path() {
    // Use ~/.kumo/config_v2.json on all platforms
    const char* home = getenv("USERPROFILE");
    if (!home) home = getenv("HOME");
    if (!home) home = ".";
    auto config_dir = std::filesystem::path(home) / ".kumo";
    std::filesystem::create_directories(config_dir);
    m_inner->config_path = config_dir / "config_v2.json";

    if (std::filesystem::exists(m_inner->config_path)) {
        try {
            std::ifstream file(m_inner->config_path);
            json j;
            file >> j;
            m_inner->config = j.get<Config>();
        } catch (...) {
            m_inner->config = Config{};
        }
    }
}

Config ConfigStore::get_config() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config;
}

json ConfigStore::get_value(const std::string& key) const {
    std::shared_lock lock(m_inner->mutex);
    json j = m_inner->config;
    json* current = &j;
    std::istringstream ss(key);
    std::string part;
    while (std::getline(ss, part, '.')) {
        if (current->is_object() && current->contains(part)) {
            current = &(*current)[part];
        } else {
            return json(nullptr);
        }
    }
    return *current;
}

bool ConfigStore::save() {
    std::shared_lock lock(m_inner->mutex);
    try {
        json j = m_inner->config;
        std::ofstream file(m_inner->config_path);
        file << j.dump(2);
        return true;
    } catch (...) {
        return false;
    }
}

std::shared_ptr<BroadcastChannel<ConfigChangedEvent>::Receiver> ConfigStore::subscribe() {
    return m_inner->changes.subscribe();
}

// Convenience methods
std::optional<Cookies> ConfigStore::get_cookies() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.cookies;
}

bool ConfigStore::set_cookies(const std::optional<Cookies>& cookies) {
    {
        std::unique_lock lock(m_inner->mutex);
        m_inner->config.cookies = cookies;
        m_inner->config.login = cookies.has_value();
    }
    return save();
}

std::optional<RoomId> ConfigStore::get_room() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.room;
}

bool ConfigStore::set_room(const RoomId& room) {
    {
        std::unique_lock lock(m_inner->mutex);
        m_inner->config.room = room;
    }
    return save();
}

float ConfigStore::get_opacity() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.opacity;
}

float ConfigStore::get_font_size() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.font_size;
}

std::string ConfigStore::get_theme() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.theme;
}

bool ConfigStore::get_lite_mode() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.lite_mode;
}

bool ConfigStore::get_medal_display() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.medal_display;
}

bool ConfigStore::get_interact_display() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.interact_display;
}

bool ConfigStore::get_guard_effect() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.guard_effect;
}

bool ConfigStore::get_level_effect() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.level_effect;
}

WindowConfig ConfigStore::get_window_config(WindowType window_type) const {
    std::string key = window_type_name(window_type);
    std::shared_lock lock(m_inner->mutex);
    auto it = m_inner->config.windows.find(key);
    if (it != m_inner->config.windows.end()) return it->second;
    return {};
}

bool ConfigStore::set_window_config(WindowType window_type, const WindowConfig& cfg) {
    {
        std::string key = window_type_name(window_type);
        std::unique_lock lock(m_inner->mutex);
        m_inner->config.windows[key] = cfg;
    }
    return save();
}

bool ConfigStore::is_always_on_top() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.always_on_top;
}

bool ConfigStore::set_always_on_top(bool value) {
    {
        std::unique_lock lock(m_inner->mutex);
        m_inner->config.always_on_top = value;
    }
    return save();
}

bool ConfigStore::is_merge_enabled() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.merge;
}

std::vector<RoomId> ConfigStore::get_merge_rooms() const {
    std::shared_lock lock(m_inner->mutex);
    return m_inner->config.merge_rooms;
}

std::filesystem::path ConfigStore::data_dir() const {
    return m_inner->config_path.parent_path();
}

} // namespace kumo
