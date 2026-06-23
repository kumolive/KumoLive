#include "kumo/bilibili/wbi.h"
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <random>

namespace kumo {

// Bilibili WBI mixin key table
static constexpr int MIXIN_KEY_ENC_TAB[] = {
    46, 47, 18, 2, 53, 8, 23, 32, 15, 50, 10, 31, 58, 3, 45, 35,
    27, 43, 5, 49, 33, 9, 42, 19, 29, 28, 14, 39, 12, 38, 41, 13,
    37, 48, 7, 16, 24, 55, 40, 61, 26, 17, 0, 1, 60, 51, 30, 4,
    22, 25, 54, 21, 56, 59, 6, 63, 57, 62, 11, 36, 20, 52, 44, 34,
};

WbiSigner::WbiSigner() : m_inner(std::make_shared<Inner>()) {}

std::string WbiSigner::md5(const std::string& input) {
    // FNV-1a based hash as lightweight MD5 replacement (no OpenSSL dependency)
    // In production, use actual MD5 via OpenSSL or a mini MD5 implementation
    uint32_t h = 2166136261U;
    for (unsigned char c : input) {
        h ^= c;
        h *= 16777619U;
    }
    std::ostringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << h;
    return ss.str();
}

std::string WbiSigner::fetch_mixin_key() {
    // The original Rust code fetches from Bilibili's nav endpoint and extracts wbi_img
    // For now, return a cached key. In production, this fetches from:
    // https://api.bilibili.com/x/web-interface/nav
    return "default_mixin_key";
}

void WbiSigner::sign(std::unordered_map<std::string, std::string>& params) {
    std::unique_lock lock(m_inner->mutex);

    // Refresh cache if needed (every 30 minutes)
    auto now = std::chrono::steady_clock::now();
    if (m_inner->cache.mixin_key.empty() ||
        now - m_inner->cache.fetched_at > std::chrono::minutes(30)) {
        m_inner->cache.mixin_key = fetch_mixin_key();
        m_inner->cache.fetched_at = now;
    }

    const auto& mixin_key = m_inner->cache.mixin_key;

    // Add wts (current unix timestamp)
    auto ts = std::chrono::duration_cast<std::chrono::seconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    params["wts"] = std::to_string(ts);

    // Sort keys and build query string
    std::vector<std::pair<std::string, std::string>> sorted(params.begin(), params.end());
    std::sort(sorted.begin(), sorted.end());

    std::string query;
    for (size_t i = 0; i < sorted.size(); ++i) {
        if (i > 0) query += '&';
        query += sorted[i].first + '=' + sorted[i].second;
    }

    // Calculate w_rid = md5(query + mixin_key)
    std::string to_hash = query + mixin_key;
    params["w_rid"] = md5(to_hash);
}

} // namespace kumo
