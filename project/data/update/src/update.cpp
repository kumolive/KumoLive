#include "kumo/update/update.h"
#include <nlohmann/json.hpp>
#include <sstream>
#include <vector>

namespace kumo {

bool compare_versions(const std::string& current, const std::string& latest) {
    auto parse = [](const std::string& v) -> std::vector<uint32_t> {
        std::vector<uint32_t> parts;
        std::istringstream ss(v);
        std::string segment;
        while (std::getline(ss, segment, '.')) {
            try { parts.push_back(static_cast<uint32_t>(std::stoul(segment))); }
            catch (...) { parts.push_back(0); }
        }
        return parts;
    };

    auto cp = parse(current);
    auto lp = parse(latest);

    for (size_t i = 0; i < 3; ++i) {
        uint32_t c = i < cp.size() ? cp[i] : 0;
        uint32_t l = i < lp.size() ? lp[i] : 0;
        if (l > c) return true;
        if (l < c) return false;
    }
    return false;
}

UpdateInfo check_for_update(const std::string& current_version) {
    UpdateInfo info;
    info.current_version = current_version;
    info.has_update = false;

    // In production:
    // 1. HTTP GET https://api.github.com/repos/Xinrea/kumo/releases/latest
    // 2. Parse GitHubRelease from JSON
    // 3. Compare tag_name with current_version

    // Stub: no update available
    return info;
}

} // namespace kumo
