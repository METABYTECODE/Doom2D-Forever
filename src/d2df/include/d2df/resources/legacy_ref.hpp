#pragma once

#include <string>
#include <string_view>

namespace d2df::resources {

/// Normalize legacy WAD path for alias lookup (slashes, lowercase archive).
[[nodiscard]] std::string normalize_legacy_ref(std::string_view ref);

/// Archive portion of a legacy ref (`game.WAD:TEXTURES/FOO` -> `game.WAD`).
[[nodiscard]] std::string extract_archive_from_legacy_ref(std::string_view ref);

/// Resolve map-relative refs (`:MUS_DOOM/E1M6`) against the map WAD legacy ref.
[[nodiscard]] std::string qualify_map_legacy_ref(std::string_view ref, std::string_view map_legacy_ref);

/// Built-in liquid textures from MAPDEF.pas — not content assets.
[[nodiscard]] bool is_builtin_texture_ref(std::string_view ref);

} // namespace d2df::resources
