#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>

namespace d2df::tools {

/// Decode CP1251 bytes to UTF-8. Returns nullopt if invalid for strict mode.
[[nodiscard]] std::string cp1251_to_utf8(std::string_view input);

/// Best-effort decode: CP1251 first, fallback Latin-1.
[[nodiscard]] std::string decode_legacy_text(std::string_view bytes);

/// Decode UTF-8 where Cyrillic was stored as Latin-1 supplement (U+0080..U+00FF) code points.
/// Returns nullopt when the string already contains non-Latin-1 Unicode (proper UTF-8 Cyrillic).
[[nodiscard]] std::optional<std::string> utf8_latin1_mojibake_to_utf8(std::string_view utf8);

} // namespace d2df::tools
