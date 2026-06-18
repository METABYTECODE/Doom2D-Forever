#include <rivet/resources/resource_manager.hpp>

#include <rivet/render/irenderer.hpp>

namespace rivet::resources {

namespace {

[[nodiscard]] std::vector<std::uint8_t> make_checkerboard_pixels(const int size, const int cell_pixels) {
    std::vector<std::uint8_t> pixels(static_cast<std::size_t>(size) * static_cast<std::size_t>(size) * 4);
    for (int y = 0; y < size; ++y) {
        for (int x = 0; x < size; ++x) {
            const bool light = ((x / cell_pixels) + (y / cell_pixels)) % 2 == 0;
            const std::uint8_t value = light ? 230 : 70;
            const std::size_t index =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(size) + static_cast<std::size_t>(x)) * 4;
            pixels[index + 0] = value;
            pixels[index + 1] = light ? 120 : 40;
            pixels[index + 2] = light ? 80 : 20;
            pixels[index + 3] = 255;
        }
    }
    return pixels;
}

} // namespace

void ResourceManager::bind_renderer(render::IRenderer* const renderer) {
    renderer_ = renderer;
}

TextureHandle ResourceManager::load_texture(const std::filesystem::path& path) {
    if (renderer_ == nullptr) {
        return kInvalidTexture;
    }

    const TextureHandle handle = renderer_->load_texture(path);
    if (handle == kInvalidTexture) {
        return kInvalidTexture;
    }

    if (const auto info = renderer_->texture_info(handle)) {
        textures_[handle] = *info;
        names_[handle] = path.string();
    }
    return handle;
}

TextureHandle ResourceManager::create_texture_rgba(
    const std::span<const std::uint8_t> pixels,
    const int width,
    const int height,
    std::string debug_name) {
    if (renderer_ == nullptr || width <= 0 || height <= 0) {
        return kInvalidTexture;
    }

    const TextureHandle handle = renderer_->create_texture_rgba(pixels, width, height);
    if (handle == kInvalidTexture) {
        return kInvalidTexture;
    }

    textures_[handle] = TextureInfo{width, height};
    names_[handle] = std::move(debug_name);
    return handle;
}

TextureHandle ResourceManager::create_checkerboard(const int size, const int cell_pixels, std::string debug_name) {
    const auto pixels = make_checkerboard_pixels(size, cell_pixels);
    if (debug_name.empty()) {
        debug_name = "checkerboard";
    }
    return create_texture_rgba(pixels, size, size, std::move(debug_name));
}

void ResourceManager::unload(const TextureHandle handle) {
    if (renderer_ != nullptr) {
        renderer_->unload_texture(handle);
    }
    textures_.erase(handle);
    names_.erase(handle);
}

void ResourceManager::clear() {
    if (renderer_ != nullptr) {
        for (const auto& [handle, _] : textures_) {
            renderer_->unload_texture(handle);
        }
    }
    textures_.clear();
    names_.clear();
}

bool ResourceManager::exists(const TextureHandle handle) const {
    return textures_.contains(handle);
}

std::optional<TextureInfo> ResourceManager::texture_info(const TextureHandle handle) const {
    const auto it = textures_.find(handle);
    if (it == textures_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace rivet::resources
