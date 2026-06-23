#pragma once

#include "kumo/messages/messages.h"
#include <memory>
#include <shared_mutex>
#include <vector>
#include <string>
#include <filesystem>

struct sqlite3;

namespace kumo {

// ============================================================================
// GiftStats
// ============================================================================

struct GiftStats {
    uint64_t total_paid_gifts = 0;
    uint64_t total_guards = 0;
    uint64_t total_superchats = 0;

    double total_value_cny() const;
};

// ============================================================================
// TimeBasedStats
// ============================================================================

struct TimeBasedStats {
    uint64_t danmu_count = 0;
    uint64_t gift_count = 0;
    uint64_t gift_value = 0;       // in 1/1000 yuan
    uint64_t superchat_count = 0;
    uint64_t superchat_value = 0;  // in yuan

    double gift_value_cny() const;
    double superchat_value_cny() const;
};

// ============================================================================
// TimeSeriesPoint
// ============================================================================

struct TimeSeriesPoint {
    int64_t timestamp = 0;
    uint64_t danmu_count = 0;
    uint64_t gift_value = 0;       // in 1/1000 yuan
    uint64_t superchat_value = 0;  // in yuan

    double gift_value_cny() const;
    double superchat_value_cny() const;
};

// ============================================================================
// Database
// ============================================================================

class Database {
public:
    explicit Database(const std::filesystem::path& path);
    static Database in_memory();
    ~Database();

    Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;
    Database(Database&&) = default;
    Database& operator=(Database&&) = default;

    // Insert
    bool insert_danmu(uint64_t room_id, const DanmuMessage& danmu);
    bool insert_danmus_batch(uint64_t room_id, const std::vector<DanmuMessage>& danmus);
    bool insert_gift(const GiftMessage& gift);
    bool insert_gifts_batch(const std::vector<GiftMessage>& gifts);
    bool insert_guard(const GuardMessage& guard);
    bool insert_superchat(const SuperChatMessage& sc);

    // Query
    std::vector<GiftMessage> get_recent_gifts(uint64_t room_id, size_t limit) const;
    std::vector<GuardMessage> get_recent_guards(uint64_t room_id, size_t limit) const;
    std::vector<SuperChatMessage> get_recent_superchats(uint64_t room_id, size_t limit) const;
    std::vector<DanmuMessage> get_recent_danmus(uint64_t room_id, size_t limit) const;
    std::vector<DanmuMessage> get_danmus_since(uint64_t room_id, int64_t minutes) const;
    std::vector<std::pair<std::string, int64_t>> get_danmus_by_user(uint64_t room_id, uint64_t uid, size_t limit) const;

    // Archive
    bool set_gift_archived(const std::string& id, bool archived);
    bool set_guard_archived(const std::string& id, bool archived);
    bool set_superchat_archived(const std::string& id, bool archived);

    // Delete
    bool delete_gift(const std::string& id);
    bool delete_guard(const std::string& id);
    bool delete_superchat(const std::string& id);
    bool clear_gifts(uint64_t room_id);
    bool clear_guards(uint64_t room_id);
    bool clear_superchats(uint64_t room_id);
    bool clear_room_data(uint64_t room_id);

    // Statistics
    GiftStats get_gift_stats(uint64_t room_id) const;
    TimeBasedStats get_time_based_stats(uint64_t room_id, int64_t since_timestamp) const;
    TimeBasedStats get_time_based_stats_range(uint64_t room_id, int64_t start, int64_t end) const;
    std::vector<TimeSeriesPoint> get_time_series_stats(uint64_t room_id, int64_t since, int64_t bucket_seconds) const;
    std::vector<TimeSeriesPoint> get_time_series_stats_range(uint64_t room_id, int64_t start, int64_t end, int64_t bucket_seconds) const;

private:
    struct Inner {
        sqlite3* conn = nullptr;
        mutable std::shared_mutex mutex;
    };
    std::shared_ptr<Inner> m_inner;

    void init_tables();
};

} // namespace kumo
