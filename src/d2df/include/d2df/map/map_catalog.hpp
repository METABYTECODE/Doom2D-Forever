#pragma once

#include <filesystem>
#include <vector>

namespace d2df::map {

[[nodiscard]] std::vector<std::filesystem::path> list_map_json_files(
    const std::filesystem::path& content_root);

} // namespace d2df::map
