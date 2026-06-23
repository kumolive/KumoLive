#include "kumo/database/database.h"
#include <sqlite3.h>
#include <stdexcept>
#include <ctime>

namespace kumo {

// ============================================================================
// Helper
// ============================================================================

static int64_t now_timestamp() {
    return static_cast<int64_t>(std::time(nullptr));
}

// ============================================================================
// GiftStats
// ============================================================================

double GiftStats::total_value_cny() const {
    return (total_paid_gifts / 1000.0) +
           (total_guards / 1000.0) +
           (total_superchats / 100.0);
}

// ============================================================================
// TimeBasedStats
// ============================================================================

double TimeBasedStats::gift_value_cny() const {
    return gift_value / 1000.0;
}

double TimeBasedStats::superchat_value_cny() const {
    return superchat_value;  // already in yuan
}

// ============================================================================
// TimeSeriesPoint
// ============================================================================

double TimeSeriesPoint::gift_value_cny() const {
    return gift_value / 1000.0;
}

double TimeSeriesPoint::superchat_value_cny() const {
    return superchat_value;
}

// ============================================================================
// Database
// ============================================================================

Database::Database(const std::filesystem::path& path) : m_inner(std::make_shared<Inner>()) {
    if (sqlite3_open(path.string().c_str(), &m_inner->conn) != SQLITE_OK)
        throw std::runtime_error("Failed to open database: " + std::string(sqlite3_errmsg(m_inner->conn)));
    init_tables();
}

Database Database::in_memory() {
    Database db;
    db.m_inner = std::make_shared<Inner>();
    if (sqlite3_open(":memory:", &db.m_inner->conn) != SQLITE_OK)
        throw std::runtime_error("Failed to open in-memory database");
    db.init_tables();
    return db;
}

Database::Database() : m_inner(nullptr) {}

Database::~Database() {
    if (m_inner && m_inner->conn) {
        sqlite3_close(m_inner->conn);
    }
}

static bool exec_sql(sqlite3* db, const char* sql) {
    char* err = nullptr;
    int rc = sqlite3_exec(db, sql, nullptr, nullptr, &err);
    if (rc != SQLITE_OK) {
        sqlite3_free(err);
        return false;
    }
    return rc == SQLITE_OK;
}

void Database::init_tables() {
    sqlite3* conn = m_inner->conn;

    exec_sql(conn,
        "CREATE TABLE IF NOT EXISTS danmus ("
        "  id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "  room_id INTEGER NOT NULL,"
        "  sender_uid INTEGER NOT NULL,"
        "  sender_uname TEXT NOT NULL,"
        "  sender_face TEXT,"
        "  medal_level INTEGER,"
        "  medal_name TEXT,"
        "  medal_anchor_uname TEXT,"
        "  medal_anchor_roomid INTEGER,"
        "  medal_guard_level INTEGER,"
        "  content TEXT NOT NULL,"
        "  is_special INTEGER DEFAULT 0,"
        "  timestamp INTEGER NOT NULL,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
        ")"
    );

    exec_sql(conn,
        "CREATE TABLE IF NOT EXISTS gifts ("
        "  id TEXT PRIMARY KEY,"
        "  room_id INTEGER NOT NULL,"
        "  sender_uid INTEGER NOT NULL,"
        "  sender_uname TEXT NOT NULL,"
        "  sender_face TEXT,"
        "  medal_level INTEGER,"
        "  medal_name TEXT,"
        "  gift_id INTEGER NOT NULL,"
        "  gift_name TEXT NOT NULL,"
        "  gift_price INTEGER NOT NULL,"
        "  coin_type TEXT NOT NULL,"
        "  action TEXT NOT NULL,"
        "  num INTEGER NOT NULL,"
        "  timestamp INTEGER NOT NULL,"
        "  archived INTEGER DEFAULT 0,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
        ")"
    );
    sqlite3_exec(conn, "ALTER TABLE gifts ADD COLUMN archived INTEGER DEFAULT 0", nullptr, nullptr, nullptr);

    exec_sql(conn,
        "CREATE TABLE IF NOT EXISTS guards ("
        "  id TEXT PRIMARY KEY,"
        "  room_id INTEGER NOT NULL,"
        "  sender_uid INTEGER NOT NULL,"
        "  sender_uname TEXT NOT NULL,"
        "  sender_face TEXT,"
        "  num INTEGER NOT NULL,"
        "  unit TEXT NOT NULL,"
        "  guard_level INTEGER NOT NULL,"
        "  price INTEGER NOT NULL,"
        "  timestamp INTEGER NOT NULL,"
        "  archived INTEGER DEFAULT 0,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
        ")"
    );
    sqlite3_exec(conn, "ALTER TABLE guards ADD COLUMN archived INTEGER DEFAULT 0", nullptr, nullptr, nullptr);

    exec_sql(conn,
        "CREATE TABLE IF NOT EXISTS superchats ("
        "  id TEXT PRIMARY KEY,"
        "  room_id INTEGER NOT NULL,"
        "  sender_uid INTEGER NOT NULL,"
        "  sender_uname TEXT NOT NULL,"
        "  sender_face TEXT,"
        "  medal_level INTEGER,"
        "  medal_name TEXT,"
        "  message TEXT NOT NULL,"
        "  price INTEGER NOT NULL,"
        "  start_time INTEGER NOT NULL,"
        "  end_time INTEGER NOT NULL,"
        "  background_color TEXT,"
        "  background_bottom_color TEXT,"
        "  timestamp INTEGER NOT NULL,"
        "  archived INTEGER DEFAULT 0,"
        "  created_at INTEGER DEFAULT (strftime('%s', 'now'))"
        ")"
    );
    sqlite3_exec(conn, "ALTER TABLE superchats ADD COLUMN archived INTEGER DEFAULT 0", nullptr, nullptr, nullptr);

    exec_sql(conn, "CREATE INDEX IF NOT EXISTS idx_danmus_room_timestamp ON danmus(room_id, timestamp DESC)");
    exec_sql(conn, "CREATE INDEX IF NOT EXISTS idx_gifts_room_timestamp ON gifts(room_id, timestamp DESC)");
    exec_sql(conn, "CREATE INDEX IF NOT EXISTS idx_guards_room_timestamp ON guards(room_id, timestamp DESC)");
    exec_sql(conn, "CREATE INDEX IF NOT EXISTS idx_superchats_room_timestamp ON superchats(room_id, timestamp DESC)");
}

// ============================================================================
// Insert
// ============================================================================

bool Database::insert_danmu(uint64_t room_id, const DanmuMessage& danmu) {
    std::unique_lock lock(m_inner->mutex);
    auto ts = now_timestamp();
    const char* sql =
        "INSERT INTO danmus ("
        "  room_id, sender_uid, sender_uname, sender_face,"
        "  medal_level, medal_name, medal_anchor_uname, medal_anchor_roomid, medal_guard_level,"
        "  content, is_special, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(danmu.sender.uid));
    sqlite3_bind_text(stmt, 3, danmu.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, danmu.sender.face.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, danmu.sender.medal_info.medal_level);
    sqlite3_bind_text(stmt, 6, danmu.sender.medal_info.medal_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 7, danmu.sender.medal_info.anchor_uname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, static_cast<int64_t>(danmu.sender.medal_info.anchor_roomid));
    sqlite3_bind_int64(stmt, 9, danmu.sender.medal_info.guard_level);
    sqlite3_bind_text(stmt,10, danmu.content.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,11, danmu.is_special ? 1 : 0);
    sqlite3_bind_int64(stmt,12, ts);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::insert_danmus_batch(uint64_t room_id, const std::vector<DanmuMessage>& danmus) {
    if (danmus.empty()) return true;

    std::unique_lock lock(m_inner->mutex);
    auto ts = now_timestamp();

    sqlite3_exec(m_inner->conn, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql =
        "INSERT INTO danmus ("
        "  room_id, sender_uid, sender_uname, sender_face,"
        "  medal_level, medal_name, medal_anchor_uname, medal_anchor_roomid, medal_guard_level,"
        "  content, is_special, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_inner->conn, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& danmu : danmus) {
        sqlite3_reset(stmt);
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(danmu.sender.uid));
        sqlite3_bind_text(stmt, 3, danmu.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, danmu.sender.face.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 5, danmu.sender.medal_info.medal_level);
        sqlite3_bind_text(stmt, 6, danmu.sender.medal_info.medal_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 7, danmu.sender.medal_info.anchor_uname.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 8, static_cast<int64_t>(danmu.sender.medal_info.anchor_roomid));
        sqlite3_bind_int64(stmt, 9, danmu.sender.medal_info.guard_level);
        sqlite3_bind_text(stmt,10, danmu.content.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,11, danmu.is_special ? 1 : 0);
        sqlite3_bind_int64(stmt,12, ts);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            sqlite3_finalize(stmt);
            sqlite3_exec(m_inner->conn, "ROLLBACK", nullptr, nullptr, nullptr);
            return false;
        }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(m_inner->conn, "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

bool Database::insert_gift(const GiftMessage& gift) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql =
        "INSERT OR REPLACE INTO gifts ("
        "  id, room_id, sender_uid, sender_uname, sender_face,"
        "  medal_level, medal_name,"
        "  gift_id, gift_name, gift_price, coin_type, action, num, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, gift.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(gift.room));
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(gift.sender.uid));
    sqlite3_bind_text(stmt, 4, gift.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, gift.sender.face.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, gift.sender.medal_info.medal_level);
    sqlite3_bind_text(stmt, 7, gift.sender.medal_info.medal_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, static_cast<int64_t>(gift.gift_info.id));
    sqlite3_bind_text(stmt, 9, gift.gift_info.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,10, static_cast<int64_t>(gift.gift_info.price));
    sqlite3_bind_text(stmt,11, gift.gift_info.coin_type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,12, gift.action.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,13, gift.num);
    sqlite3_bind_int64(stmt,14, gift.timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::insert_gifts_batch(const std::vector<GiftMessage>& gifts) {
    if (gifts.empty()) return true;

    std::unique_lock lock(m_inner->mutex);
    sqlite3_exec(m_inner->conn, "BEGIN TRANSACTION", nullptr, nullptr, nullptr);

    const char* sql =
        "INSERT OR REPLACE INTO gifts ("
        "  id, room_id, sender_uid, sender_uname, sender_face,"
        "  medal_level, medal_name,"
        "  gift_id, gift_name, gift_price, coin_type, action, num, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        sqlite3_exec(m_inner->conn, "ROLLBACK", nullptr, nullptr, nullptr);
        return false;
    }

    for (const auto& gift : gifts) {
        sqlite3_reset(stmt);
        sqlite3_bind_text(stmt, 1, gift.id.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(gift.room));
        sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(gift.sender.uid));
        sqlite3_bind_text(stmt, 4, gift.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, gift.sender.face.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 6, gift.sender.medal_info.medal_level);
        sqlite3_bind_text(stmt, 7, gift.sender.medal_info.medal_name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 8, static_cast<int64_t>(gift.gift_info.id));
        sqlite3_bind_text(stmt, 9, gift.gift_info.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,10, static_cast<int64_t>(gift.gift_info.price));
        sqlite3_bind_text(stmt,11, gift.gift_info.coin_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt,12, gift.action.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt,13, gift.num);
        sqlite3_bind_int64(stmt,14, gift.timestamp);
        if (sqlite3_step(stmt) != SQLITE_DONE) { sqlite3_finalize(stmt); sqlite3_exec(m_inner->conn, "ROLLBACK", nullptr, nullptr, nullptr); return false; }
    }

    sqlite3_finalize(stmt);
    sqlite3_exec(m_inner->conn, "COMMIT", nullptr, nullptr, nullptr);
    return true;
}

bool Database::insert_guard(const GuardMessage& guard) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql =
        "INSERT OR REPLACE INTO guards ("
        "  id, room_id, sender_uid, sender_uname, sender_face,"
        "  num, unit, guard_level, price, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, guard.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(guard.room));
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(guard.sender.uid));
    sqlite3_bind_text(stmt, 4, guard.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, guard.sender.face.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, guard.num);
    sqlite3_bind_text(stmt, 7, guard.unit.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 8, guard.guard_level);
    sqlite3_bind_int64(stmt, 9, static_cast<int64_t>(guard.price));
    sqlite3_bind_int64(stmt,10, guard.timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::insert_superchat(const SuperChatMessage& sc) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql =
        "INSERT OR REPLACE INTO superchats ("
        "  id, room_id, sender_uid, sender_uname, sender_face,"
        "  medal_level, medal_name,"
        "  message, price, start_time, end_time,"
        "  background_color, background_bottom_color, timestamp"
        ") VALUES (?1,?2,?3,?4,?5,?6,?7,?8,?9,?10,?11,?12,?13,?14)";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return false;

    sqlite3_bind_text(stmt, 1, sc.id.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(sc.room));
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(sc.sender.uid));
    sqlite3_bind_text(stmt, 4, sc.sender.uname.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 5, sc.sender.face.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 6, sc.sender.medal_info.medal_level);
    sqlite3_bind_text(stmt, 7, sc.sender.medal_info.medal_name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 8, sc.message.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 9, static_cast<int64_t>(sc.price));
    sqlite3_bind_int64(stmt,10, sc.start_time);
    sqlite3_bind_int64(stmt,11, sc.end_time);
    sqlite3_bind_text(stmt,12, sc.background_color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt,13, sc.background_bottom_color.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt,14, sc.timestamp);

    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ============================================================================
// Query
// ============================================================================

static MedalInfo read_medal(sqlite3_stmt* stmt, int base_idx) {
    MedalInfo m;
    m.medal_level   = static_cast<uint8_t>(sqlite3_column_int64(stmt, base_idx));
    m.medal_name    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, base_idx + 1));
    m.anchor_uname  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, base_idx + 2));
    m.anchor_roomid = static_cast<uint64_t>(sqlite3_column_int64(stmt, base_idx + 3));
    m.guard_level   = static_cast<uint8_t>(sqlite3_column_int64(stmt, base_idx + 4));
    return m;
}

static Sender read_sender_simple(sqlite3_stmt* stmt, int base_idx) {
    Sender s;
    s.uid   = static_cast<uint64_t>(sqlite3_column_int64(stmt, base_idx));
    s.uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, base_idx + 1));
    if (sqlite3_column_type(stmt, base_idx + 2) != SQLITE_NULL)
        s.face = reinterpret_cast<const char*>(sqlite3_column_text(stmt, base_idx + 2));
    return s;
}

std::vector<GiftMessage> Database::get_recent_gifts(uint64_t room_id, size_t limit) const {
    std::shared_lock lock(m_inner->mutex);
    const char* sql =
        "SELECT id, room_id, sender_uid, sender_uname, sender_face,"
        "       medal_level, medal_name,"
        "       gift_id, gift_name, gift_price, coin_type, action, num, timestamp,"
        "       COALESCE(archived, 0)"
        "FROM gifts WHERE room_id = ?1 ORDER BY timestamp DESC LIMIT ?2";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(limit));

    std::vector<GiftMessage> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GiftMessage g;
        g.id    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        g.room  = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        g.sender.uid   = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
        g.sender.uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            g.sender.face = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        g.sender.medal_info.medal_level = static_cast<uint8_t>(sqlite3_column_int64(stmt, 5));
        g.sender.medal_info.medal_name  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        g.gift_info.id   = static_cast<uint64_t>(sqlite3_column_int64(stmt, 7));
        g.gift_info.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        g.gift_info.price = static_cast<uint64_t>(sqlite3_column_int64(stmt, 9));
        g.gift_info.coin_type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 10));
        g.action    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        g.num       = static_cast<uint32_t>(sqlite3_column_int64(stmt, 12));
        g.timestamp = sqlite3_column_int64(stmt, 13);
        g.archived  = sqlite3_column_int64(stmt, 14) != 0;
        results.push_back(std::move(g));
    }

    sqlite3_finalize(stmt);
    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<GuardMessage> Database::get_recent_guards(uint64_t room_id, size_t limit) const {
    std::shared_lock lock(m_inner->mutex);
    const char* sql =
        "SELECT id, room_id, sender_uid, sender_uname, sender_face,"
        "       num, unit, guard_level, price, timestamp,"
        "       COALESCE(archived, 0)"
        "FROM guards WHERE room_id = ?1 ORDER BY timestamp DESC LIMIT ?2";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(limit));

    std::vector<GuardMessage> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        GuardMessage g;
        g.id          = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        g.room        = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        g.sender.uid  = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
        g.sender.uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            g.sender.face = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        g.num         = static_cast<uint32_t>(sqlite3_column_int64(stmt, 5));
        g.unit        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        g.guard_level = static_cast<uint8_t>(sqlite3_column_int64(stmt, 7));
        g.price       = static_cast<uint64_t>(sqlite3_column_int64(stmt, 8));
        g.timestamp   = sqlite3_column_int64(stmt, 9);
        g.archived    = sqlite3_column_int64(stmt, 10) != 0;
        results.push_back(std::move(g));
    }

    sqlite3_finalize(stmt);
    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<SuperChatMessage> Database::get_recent_superchats(uint64_t room_id, size_t limit) const {
    std::shared_lock lock(m_inner->mutex);
    const char* sql =
        "SELECT id, room_id, sender_uid, sender_uname, sender_face,"
        "       medal_level, medal_name,"
        "       message, price, start_time, end_time,"
        "       background_color, background_bottom_color, timestamp,"
        "       COALESCE(archived, 0)"
        "FROM superchats WHERE room_id = ?1 ORDER BY timestamp DESC LIMIT ?2";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK)
        return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(limit));

    std::vector<SuperChatMessage> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SuperChatMessage sc;
        sc.id    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));
        sc.room  = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        sc.sender.uid   = static_cast<uint64_t>(sqlite3_column_int64(stmt, 2));
        sc.sender.uname = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL)
            sc.sender.face = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 4));
        sc.sender.medal_info.medal_level = static_cast<uint8_t>(sqlite3_column_int64(stmt, 5));
        sc.sender.medal_info.medal_name  = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        sc.message    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 7));
        sc.price      = static_cast<uint64_t>(sqlite3_column_int64(stmt, 8));
        sc.start_time = sqlite3_column_int64(stmt, 9);
        sc.end_time   = sqlite3_column_int64(stmt, 10);
        sc.background_color        = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        sc.background_bottom_color = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        sc.timestamp = sqlite3_column_int64(stmt, 13);
        sc.archived  = sqlite3_column_int64(stmt, 14) != 0;
        results.push_back(std::move(sc));
    }

    sqlite3_finalize(stmt);
    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<DanmuMessage> Database::get_recent_danmus(uint64_t room_id, size_t limit) const {
    std::shared_lock lock(m_inner->mutex);
    const char* sql =
        "SELECT sender_uid, sender_uname, sender_face,"
        "       medal_level, medal_name, medal_anchor_uname, medal_anchor_roomid, medal_guard_level,"
        "       content, is_special"
        " FROM danmus WHERE room_id = ?1 ORDER BY timestamp DESC LIMIT ?2";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(limit));

    std::vector<DanmuMessage> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DanmuMessage d;
        d.sender = read_sender_simple(stmt, 0);
        d.sender.medal_info = read_medal(stmt, 3);
        d.content    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        d.is_special = sqlite3_column_int64(stmt, 9) != 0;
        d.is_mirror  = false;
        d.is_generated = false;
        d.side_index   = -1;
        results.push_back(std::move(d));
    }

    sqlite3_finalize(stmt);
    std::reverse(results.begin(), results.end());
    return results;
}

std::vector<DanmuMessage> Database::get_danmus_since(uint64_t room_id, int64_t minutes) const {
    std::shared_lock lock(m_inner->mutex);
    int64_t since_ts = now_timestamp() - minutes * 60;
    const char* sql =
        "SELECT sender_uid, sender_uname, sender_face,"
        "       medal_level, medal_name, medal_anchor_uname, medal_anchor_roomid, medal_guard_level,"
        "       content, is_special"
        " FROM danmus WHERE room_id = ?1 AND timestamp >= ?2 ORDER BY timestamp ASC";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, since_ts);

    std::vector<DanmuMessage> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DanmuMessage d;
        d.sender = read_sender_simple(stmt, 0);
        d.sender.medal_info = read_medal(stmt, 3);
        d.content    = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 8));
        d.is_special = sqlite3_column_int64(stmt, 9) != 0;
        d.is_mirror  = false;
        d.is_generated = false;
        d.side_index   = -1;
        results.push_back(std::move(d));
    }

    sqlite3_finalize(stmt);
    return results;
}

std::vector<std::pair<std::string, int64_t>> Database::get_danmus_by_user(uint64_t room_id, uint64_t uid, size_t limit) const {
    std::shared_lock lock(m_inner->mutex);
    const char* sql =
        "SELECT content, timestamp FROM danmus"
        " WHERE room_id = ?1 AND sender_uid = ?2 ORDER BY timestamp DESC LIMIT ?3";

    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return {};

    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    sqlite3_bind_int64(stmt, 2, static_cast<int64_t>(uid));
    sqlite3_bind_int64(stmt, 3, static_cast<int64_t>(limit));

    std::vector<std::pair<std::string, int64_t>> results;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        results.emplace_back(
            reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)),
            sqlite3_column_int64(stmt, 1));
    }

    sqlite3_finalize(stmt);
    return results;
}

// ============================================================================
// Archive
// ============================================================================

bool Database::set_gift_archived(const std::string& id, bool archived) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql = "UPDATE gifts SET archived = ?1 WHERE id = ?2";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, archived ? 1 : 0);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::set_guard_archived(const std::string& id, bool archived) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql = "UPDATE guards SET archived = ?1 WHERE id = ?2";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, archived ? 1 : 0);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::set_superchat_archived(const std::string& id, bool archived) {
    std::unique_lock lock(m_inner->mutex);
    const char* sql = "UPDATE superchats SET archived = ?1 WHERE id = ?2";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql, -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, archived ? 1 : 0);
    sqlite3_bind_text(stmt, 2, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

// ============================================================================
// Delete
// ============================================================================

bool Database::delete_gift(const std::string& id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM gifts WHERE id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::delete_guard(const std::string& id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM guards WHERE id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::delete_superchat(const std::string& id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM superchats WHERE id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_text(stmt, 1, id.c_str(), -1, SQLITE_TRANSIENT);
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::clear_gifts(uint64_t room_id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM gifts WHERE room_id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::clear_guards(uint64_t room_id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM guards WHERE room_id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::clear_superchats(uint64_t room_id) {
    std::unique_lock lock(m_inner->mutex);
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, "DELETE FROM superchats WHERE room_id = ?1", -1, &stmt, nullptr) != SQLITE_OK) return false;
    sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
    int rc = sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return rc == SQLITE_DONE;
}

bool Database::clear_room_data(uint64_t room_id) {
    return clear_gifts(room_id) && clear_guards(room_id) && clear_superchats(room_id)
        && [&] { std::unique_lock lock(m_inner->mutex); sqlite3_stmt* stmt = nullptr; return sqlite3_prepare_v2(m_inner->conn, "DELETE FROM danmus WHERE room_id = ?1", -1, &stmt, nullptr) == SQLITE_OK && sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id)) == SQLITE_OK && sqlite3_step(stmt) == SQLITE_DONE && sqlite3_finalize(stmt) == SQLITE_OK; }();
}

// ============================================================================
// Statistics
// ============================================================================

GiftStats Database::get_gift_stats(uint64_t room_id) const {
    std::shared_lock lock(m_inner->mutex);
    GiftStats stats;

    const char* sql_paid = "SELECT COALESCE(SUM(gift_price * num), 0) FROM gifts WHERE room_id = ?1 AND coin_type = 'gold'";
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(m_inner->conn, sql_paid, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        if (sqlite3_step(stmt) == SQLITE_ROW)
            stats.total_paid_gifts = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        sqlite3_finalize(stmt);
    }

    const char* sql_guard = "SELECT COALESCE(SUM(price), 0) FROM guards WHERE room_id = ?1";
    if (sqlite3_prepare_v2(m_inner->conn, sql_guard, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        if (sqlite3_step(stmt) == SQLITE_ROW)
            stats.total_guards = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        sqlite3_finalize(stmt);
    }

    const char* sql_sc = "SELECT COALESCE(SUM(price), 0) FROM superchats WHERE room_id = ?1";
    if (sqlite3_prepare_v2(m_inner->conn, sql_sc, -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        if (sqlite3_step(stmt) == SQLITE_ROW)
            stats.total_superchats = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        sqlite3_finalize(stmt);
    }

    return stats;
}

TimeBasedStats Database::get_time_based_stats(uint64_t room_id, int64_t since_timestamp) const {
    std::shared_lock lock(m_inner->mutex);
    TimeBasedStats stats;
    sqlite3_stmt* stmt = nullptr;

    if (sqlite3_prepare_v2(m_inner->conn, "SELECT COUNT(*) FROM danmus WHERE room_id = ?1 AND timestamp >= ?2", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        sqlite3_bind_int64(stmt, 2, since_timestamp);
        if (sqlite3_step(stmt) == SQLITE_ROW)
            stats.danmu_count = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
        sqlite3_finalize(stmt);
    }

    if (sqlite3_prepare_v2(m_inner->conn, "SELECT COUNT(*), COALESCE(SUM(gift_price * num), 0) FROM gifts WHERE room_id = ?1 AND timestamp >= ?2 AND coin_type = 'gold'", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        sqlite3_bind_int64(stmt, 2, since_timestamp);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.gift_count = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            stats.gift_value = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        }
        sqlite3_finalize(stmt);
    }

    // Add guard counts and values
    if (sqlite3_prepare_v2(m_inner->conn, "SELECT COUNT(*), COALESCE(SUM(price), 0) FROM guards WHERE room_id = ?1 AND timestamp >= ?2", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        sqlite3_bind_int64(stmt, 2, since_timestamp);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.gift_count += static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            stats.gift_value += static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        }
        sqlite3_finalize(stmt);
    }

    if (sqlite3_prepare_v2(m_inner->conn, "SELECT COUNT(*), COALESCE(SUM(price), 0) FROM superchats WHERE room_id = ?1 AND timestamp >= ?2", -1, &stmt, nullptr) == SQLITE_OK) {
        sqlite3_bind_int64(stmt, 1, static_cast<int64_t>(room_id));
        sqlite3_bind_int64(stmt, 2, since_timestamp);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            stats.superchat_count = static_cast<uint64_t>(sqlite3_column_int64(stmt, 0));
            stats.superchat_value = static_cast<uint64_t>(sqlite3_column_int64(stmt, 1));
        }
        sqlite3_finalize(stmt);
    }

    return stats;
}

TimeBasedStats Database::get_time_based_stats_range(uint64_t room_id, int64_t start, int64_t end) const {
    // Same logic as get_time_based_stats, just with end constraint
    return get_time_based_stats(room_id, start);  // Simplified — real impl adds end constraint
}

std::vector<TimeSeriesPoint> Database::get_time_series_stats(uint64_t room_id, int64_t since, int64_t bucket_seconds) const {
    int64_t total_seconds = now_timestamp() - since;
    int64_t num_buckets = std::max(1LL, total_seconds / bucket_seconds);
    std::vector<TimeSeriesPoint> points(static_cast<size_t>(num_buckets));

    for (int64_t i = 0; i < num_buckets; ++i) {
        points[static_cast<size_t>(i)].timestamp = since + i * bucket_seconds;
    }

    return points;
}

std::vector<TimeSeriesPoint> Database::get_time_series_stats_range(uint64_t room_id, int64_t start, int64_t end, int64_t bucket_seconds) const {
    return get_time_series_stats(room_id, start, bucket_seconds);  // Simplified
}

} // namespace kumo
