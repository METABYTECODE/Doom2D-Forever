#pragma once

#include <cstdint>
#include <filesystem>
#include <span>

namespace rivet::resources {

bool write_png_rgba(const std::filesystem::path& path, std::span<const std::uint8_t> pixels, int width, int height);

} // namespace rivet::resources
