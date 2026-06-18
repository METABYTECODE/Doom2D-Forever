#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

#include <filesystem>

#include <rivet/resources/png_writer.hpp>

namespace rivet::resources {

bool write_png_rgba(
    const std::filesystem::path& path,
    const std::span<const std::uint8_t> pixels,
    const int width,
    const int height) {
    if (width <= 0 || height <= 0) {
        return false;
    }

    const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    if (pixels.size() < expected) {
        return false;
    }

    std::error_code error;
    std::filesystem::create_directories(path.parent_path(), error);

    return stbi_write_png(path.string().c_str(), width, height, 4, pixels.data(), width * 4) != 0;
}

} // namespace rivet::resources
