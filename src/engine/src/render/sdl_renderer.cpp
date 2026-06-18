#define SDL_MAIN_HANDLED
#include <SDL.h>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

#include <filesystem>
#include <stdexcept>
#include <string>

#include <rivet/render/sdl_renderer.hpp>

namespace rivet::render {

namespace {

[[nodiscard]] SDL_Texture* upload_texture(
    SDL_Renderer* renderer,
    const std::uint8_t* pixels,
    const int width,
    const int height) {
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(
        const_cast<std::uint8_t*>(pixels),
        width,
        height,
        32,
        width * 4,
        SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr) {
        return nullptr;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    return texture;
}

} // namespace

SdlRenderer::SdlRenderer(SDL_Window* const window) {
    if (window == nullptr) {
        throw std::runtime_error("SdlRenderer: window is null");
    }

    renderer_ = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
    if (renderer_ == nullptr) {
        throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
    }
}

SdlRenderer::~SdlRenderer() {
    for (const auto& [handle, texture] : textures_) {
        (void)handle;
        SDL_DestroyTexture(texture);
    }
    textures_.clear();
    texture_info_.clear();

    if (renderer_ != nullptr) {
        SDL_DestroyRenderer(renderer_);
        renderer_ = nullptr;
    }
}

void SdlRenderer::begin_frame() {
    if (renderer_ == nullptr) {
        return;
    }
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
}

void SdlRenderer::end_frame() {
    if (renderer_ == nullptr) {
        return;
    }
    SDL_RenderPresent(renderer_);
}

void SdlRenderer::clear(const Color& color) {
    if (renderer_ == nullptr) {
        return;
    }
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderClear(renderer_);
}

Rect SdlRenderer::apply_camera(const Rect& world_rect) const {
    return camera_.world_to_screen(world_rect);
}

void SdlRenderer::draw_filled_rect(const Rect& rect, const Color& color) {
    if (renderer_ == nullptr) {
        return;
    }

    const Rect screen_rect = apply_camera(rect);
    const SDL_FRect target{screen_rect.x, screen_rect.y, screen_rect.width, screen_rect.height};
    SDL_SetRenderDrawColor(renderer_, color.r, color.g, color.b, color.a);
    SDL_RenderFillRectF(renderer_, &target);
}

void SdlRenderer::set_camera(const Camera2D& camera) {
    camera_ = camera;
}

resources::TextureHandle SdlRenderer::load_texture(const std::filesystem::path& path) {
    if (renderer_ == nullptr) {
        return resources::kInvalidTexture;
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels = stbi_load(path.string().c_str(), &width, &height, &channels, 4);
    if (pixels == nullptr) {
        return resources::kInvalidTexture;
    }

    SDL_Texture* texture = upload_texture(renderer_, pixels, width, height);
    stbi_image_free(pixels);
    if (texture == nullptr) {
        return resources::kInvalidTexture;
    }

    const resources::TextureHandle handle = next_texture_handle_++;
    textures_[handle] = texture;
    texture_info_[handle] = resources::TextureInfo{width, height};
    return handle;
}

resources::TextureHandle SdlRenderer::create_texture_rgba(
    const std::span<const std::uint8_t> pixels,
    const int width,
    const int height) {
    if (renderer_ == nullptr || width <= 0 || height <= 0) {
        return resources::kInvalidTexture;
    }

    const std::size_t expected = static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4;
    if (pixels.size() < expected) {
        return resources::kInvalidTexture;
    }

    SDL_Texture* texture = upload_texture(renderer_, pixels.data(), width, height);
    if (texture == nullptr) {
        return resources::kInvalidTexture;
    }

    const resources::TextureHandle handle = next_texture_handle_++;
    textures_[handle] = texture;
    texture_info_[handle] = resources::TextureInfo{width, height};
    return handle;
}

void SdlRenderer::unload_texture(const resources::TextureHandle handle) {
    const auto it = textures_.find(handle);
    if (it == textures_.end()) {
        return;
    }
    SDL_DestroyTexture(it->second);
    textures_.erase(it);
    texture_info_.erase(handle);
}

void SdlRenderer::draw_texture(const resources::TextureHandle handle, const Rect& dest) {
    if (renderer_ == nullptr) {
        return;
    }

    const auto it = textures_.find(handle);
    if (it == textures_.end()) {
        return;
    }

    const Rect screen_rect = apply_camera(dest);
    const SDL_FRect target{screen_rect.x, screen_rect.y, screen_rect.width, screen_rect.height};
    SDL_RenderCopyF(renderer_, it->second, nullptr, &target);
}

std::optional<resources::TextureInfo> SdlRenderer::texture_info(const resources::TextureHandle handle) const {
    const auto it = texture_info_.find(handle);
    if (it == texture_info_.end()) {
        return std::nullopt;
    }
    return it->second;
}

} // namespace rivet::render
