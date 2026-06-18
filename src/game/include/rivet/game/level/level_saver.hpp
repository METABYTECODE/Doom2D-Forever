#pragma once

#include <filesystem>

#include <rivet/game/level/level_data.hpp>

namespace rivet::game::level {

/// Writes a level document to disk in rivet-level JSON format.
void save_level(const std::filesystem::path& path, const LevelData& level);

} // namespace rivet::game::level
