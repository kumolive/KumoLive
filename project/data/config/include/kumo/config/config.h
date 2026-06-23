#pragma once

#include "kumo/types/types.h"
#include "kumo/events/events.h"
#include <unordered_map>
#include <shared_mutex>
#include <memory>
#include <filesystem>

namespace kumo {

// ============================================================================
// WindowConfig
// ============================================================================

struct WindowConfig {
    int32_t x = 0;
    int32_t y = 0;
    uint32_t width = 0;
    uint32_t height = 0;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(WindowConfig, x, y, width, height)

// ============================================================================
// TtsProvider
// ============================================================================

enum class TtsProvider : uint8_t {
    None = 0,
    System,
    Aliyun,
    Custom,
};

// ============================================================================
// Config
// ============================================================================

struct Config {
    uint32_t version = 2;
    std::optional<Cookies> cookies;
    std::optional<RoomId> room;
    bool login = false;
    bool merge = false;
    std::vector<RoomId> merge_rooms;
    bool always_on_top = false;
    size_t max_detail_entry = 100;
    bool guard_effect = false;
    bool level_effect = false;
    float opacity = 1.0f;
    bool lite_mode = false;
    bool medal_display = true;
    bool interact_display = false;
    std::string theme = "dark";
    float font_size = 14.0f;
    std::unordered_map<std::string, WindowConfig> windows;
    std::string log_level = "info";
    size_t max_danmu_count = 200;
    TtsProvider tts_provider = TtsProvider::None;
    std::string tts_aliyun_app_key;
    std::string tts_aliyun_access_key_id;
    std::string tts_aliyun_access_key_secret;
    std::string tts_custom_url;
    bool tts_enabled = false;
    bool tts_gift_enabled = false;
    bool tts_sc_enabled = false;
    float tts_volume = 1.0f;
    bool auto_update_check = true;
    json extra;
};

void to_json(json& j, const Config& c);
void from_json(const json& j, Config& c);

// ============================================================================
// ConfigStore
// ============================================================================

class ConfigStore {
public:
    ConfigStore();
    explicit ConfigStore(const std::filesystem::path& path);

    Config get_config() const;

    template<typename T>
    std::optional<T> get(const std::string& key) const;

    json get_value(const std::string& key) const;

    template<typename T>
    bool set(const std::string& key, const T& value);

    bool save();

    std::shared_ptr<BroadcastChannel<ConfigChangedEvent>::Receiver> subscribe();

    // Convenience methods
    std::optional<Cookies> get_cookies() const;
    bool set_cookies(const std::optional<Cookies>& cookies);
    std::optional<RoomId> get_room() const;
    bool set_room(const RoomId& room);
    float get_opacity() const;
    float get_font_size() const;
    std::string get_theme() const;
    bool get_lite_mode() const;
    bool get_medal_display() const;
    bool get_interact_display() const;
    bool get_guard_effect() const;
    bool get_level_effect() const;
    WindowConfig get_window_config(WindowType window_type) const;
    bool set_window_config(WindowType window_type, const WindowConfig& cfg);
    bool is_always_on_top() const;
    bool set_always_on_top(bool value);
    bool is_merge_enabled() const;
    std::vector<RoomId> get_merge_rooms() const;
    std::filesystem::path data_dir() const;

private:
    struct Inner {
        Config config;
        std::filesystem::path config_path;
        BroadcastChannel<ConfigChangedEvent> changes;
        mutable std::shared_mutex mutex;
    };
    std::shared_ptr<Inner> m_inner;

    void init_default_path();
};

} // namespace kumo
