#pragma once

#include <d2df/map/map_document.hpp>

#include <filesystem>

namespace d2df::map {

[[nodiscard]] MapDocument load_map_json_v1(const std::filesystem::path& path);

} // namespace d2df::map
