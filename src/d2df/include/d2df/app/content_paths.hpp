#pragma once

#include <filesystem>
#include <optional>

namespace d2df {

struct ResolvedContentPaths {
    std::filesystem::path content_root;
    std::filesystem::path map_path;
};

/// Find assets/content relative to cwd or executable directory.
[[nodiscard]] std::optional<ResolvedContentPaths> resolve_content_paths(
    const std::filesystem::path& map_rel = "maps/doom2d/map01.json");

} // namespace d2df
