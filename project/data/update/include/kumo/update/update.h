#pragma once

#include <string>
#include <optional>
#include <memory>
#include <shared_mutex>

namespace kumo {

// ============================================================================
// GitHubRelease
// ============================================================================

struct GitHubRelease {
    std::string tag_name;
    std::string name;
    std::string html_url;
    std::string body;
    std::string published_at;
};

// ============================================================================
// UpdateInfo
// ============================================================================

struct UpdateInfo {
    std::string current_version;
    std::string latest_version;
    std::string release_url;
    std::string release_notes;
    bool has_update = false;
};

// ============================================================================
// Update checker
// ============================================================================

// Check for updates from GitHub releases asynchronously
// current_version should be the app version without 'v' prefix (e.g. "3.0.0")
UpdateInfo check_for_update(const std::string& current_version);

// Compare semantic versions. Returns true if latest > current.
bool compare_versions(const std::string& current, const std::string& latest);

} // namespace kumo
