#pragma once

#include <filesystem>
#include <vector>

namespace d2df::map {

[[nodiscard]] std::vector<std::filesystem::path> list_map_json_files(
    const std::filesystem::path& content_root);

/// Lists flat native d2df-map files from assets/maps (or given directory).
[[nodiscard]] std::vector<std::filesystem::path> list_native_map_files(
    const std::filesystem::path& maps_root);

} // namespace d2df::map
