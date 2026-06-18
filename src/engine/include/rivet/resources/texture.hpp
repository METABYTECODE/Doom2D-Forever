#pragma once

#include <cstdint>

namespace rivet::resources {

using TextureHandle = std::uint32_t;
inline constexpr TextureHandle kInvalidTexture = 0;

struct TextureInfo {
    int width = 0;
    int height = 0;
};

} // namespace rivet::resources
