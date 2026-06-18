#pragma once

#include <filesystem>

#include <rivet/game/level/level_data.hpp>

namespace rivet::game::level {

/// Loads a level document from disk. Extension is arbitrary; content must be rivet-level JSON.
[[nodiscard]] LevelData load_level(const std::filesystem::path& path);

} // namespace rivet::game::level
