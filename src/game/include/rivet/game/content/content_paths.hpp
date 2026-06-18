#pragma once

#include <filesystem>
#include <optional>

namespace rivet::game::content {

/// Resolved `assets/` directory (tilesets, levels, audio, …).
[[nodiscard]] std::optional<std::filesystem::path> resolve_assets_root();

[[nodiscard]] std::filesystem::path tilesets_dir(const std::filesystem::path& assets_root);

} // namespace rivet::game::content
