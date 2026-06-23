#pragma once

#include <string>
#include <unordered_map>
#include <chrono>
#include <shared_mutex>
#include <memory>

namespace kumo {

// ============================================================================
// WbiSigner — Bilibili WBI signing
// ============================================================================

class WbiSigner {
public:
    WbiSigner();

    // Sign query parameters with WBI (modifies params in-place)
    void sign(std::unordered_map<std::string, std::string>& params);

private:
    struct Cache {
        std::string mixin_key;
        std::chrono::steady_clock::time_point fetched_at;
    };

    struct Inner {
        Cache cache;
        std::shared_mutex mutex;
    };
    std::shared_ptr<Inner> m_inner;

    std::string fetch_mixin_key();
    std::string md5(const std::string& input);
};

} // namespace kumo
