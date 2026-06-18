#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

#include <rivet/resources/texture.hpp>

namespace rivet::render {
class IRenderer;
}

namespace rivet::resources {

/// Asset registry. GPU upload is delegated to the active renderer backend.
class ResourceManager {
public:
    void bind_renderer(render::IRenderer* renderer);

    [[nodiscard]] TextureHandle load_texture(const std::filesystem::path& path);
    [[nodiscard]] TextureHandle create_texture_rgba(
        std::span<const std::uint8_t> pixels,
        int width,
        int height,
        std::string debug_name = {});
    [[nodiscard]] TextureHandle create_checkerboard(int size, int cell_pixels, std::string debug_name = {});

    void unload(TextureHandle handle);
    void clear();

    [[nodiscard]] bool exists(TextureHandle handle) const;
    [[nodiscard]] std::optional<TextureInfo> texture_info(TextureHandle handle) const;

private:
    render::IRenderer* renderer_ = nullptr;
    std::unordered_map<TextureHandle, TextureInfo> textures_;
    std::unordered_map<TextureHandle, std::string> names_;
};

} // namespace rivet::resources
