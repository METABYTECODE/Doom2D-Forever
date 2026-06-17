#include <d2df/render/texture_cache.hpp>

#include <stdexcept>
#include <vector>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

namespace d2df::render {

TextureCache::TextureCache(SDL_Renderer* renderer, const d2df::resources::AssetDatabase& assets)
    : renderer_(renderer)
    , assets_(assets) {}

TextureCache::~TextureCache() {
    for (const auto& [_, entry] : textures_) {
        if (entry.texture != nullptr) {
            SDL_DestroyTexture(entry.texture);
        }
    }
}

void TextureCache::store_texture(std::string_view key, SDL_Texture* texture) {
    TextureEntry entry;
    entry.texture = texture;
    if (texture != nullptr) {
        SDL_QueryTexture(texture, nullptr, nullptr, &entry.width, &entry.height);
    }
    textures_.emplace(std::string(key), entry);
}

bool TextureCache::query_size(SDL_Texture* texture, int& width, int& height) const {
    if (texture == nullptr) {
        return false;
    }
    for (const auto& [_, entry] : textures_) {
        if (entry.texture == texture) {
            width = entry.width;
            height = entry.height;
            return width > 0 && height > 0;
        }
    }
    return false;
}

SDL_Texture* TextureCache::get(std::string_view asset_id) {
    if (asset_id.empty()) {
        return nullptr;
    }

    const std::string key(asset_id);
    if (const auto it = textures_.find(key); it != textures_.end()) {
        return it->second.texture;
    }

    if (asset_id.front() == '_') {
        return builtin(asset_id);
    }

    SDL_Texture* texture = load_png(asset_id);
    store_texture(key, texture);
    return texture;
}

SDL_Texture* TextureCache::builtin(std::string_view ref) {
    const std::string key(ref);
    if (const auto it = textures_.find(key); it != textures_.end()) {
        return it->second.texture;
    }

    if (ref == "_water_0") {
        TextureEntry entry;
        entry.width = 32;
        entry.height = 96;
        entry.texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                          SDL_TEXTUREACCESS_STATIC, entry.width, entry.height);
        if (entry.texture != nullptr) {
            std::vector<std::uint32_t> pixels(static_cast<std::size_t>(entry.width * entry.height),
                                              (0xFFu << 24) | (32u << 16) | (96u << 8) | 224u);
            SDL_UpdateTexture(entry.texture, nullptr, pixels.data(),
                              entry.width * sizeof(std::uint32_t));
            SDL_SetTextureBlendMode(entry.texture, SDL_BLENDMODE_BLEND);
            textures_.emplace(key, entry);
            return entry.texture;
        }
    }
    if (ref == "_water_1") {
        return make_solid(64, 192, 32, ref);
    }
    if (ref == "_water_2") {
        return make_solid(192, 192, 32, ref);
    }

    return make_solid(255, 0, 255, ref);
}

SDL_Texture* TextureCache::load_png(std::string_view asset_id) {
    const auto path = assets_.path_for(asset_id);
    if (path.empty()) {
        return make_solid(255, 0, 255, asset_id);
    }

    int width = 0;
    int height = 0;
    int channels = 0;
    stbi_uc* pixels =
        stbi_load(path.string().c_str(), &width, &height, &channels, STBI_rgb_alpha);
    if (pixels == nullptr) {
        return make_solid(255, 0, 255, asset_id);
    }

    SDL_Surface* surface =
        SDL_CreateRGBSurfaceWithFormatFrom(pixels, width, height, 32, width * 4, SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr) {
        stbi_image_free(pixels);
        return make_solid(255, 0, 255, asset_id);
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer_, surface);
    SDL_FreeSurface(surface);
    stbi_image_free(pixels);

    if (texture == nullptr) {
        return make_solid(255, 0, 255, asset_id);
    }

    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    return texture;
}

SDL_Texture* TextureCache::make_solid(std::uint8_t r, std::uint8_t g, std::uint8_t b,
                                      std::string_view key) {
    SDL_Texture* texture = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGBA8888,
                                             SDL_TEXTUREACCESS_STATIC, 1, 1);
    if (texture == nullptr) {
        return nullptr;
    }

    const std::uint32_t pixel = (0xFFu << 24) | (static_cast<std::uint32_t>(r) << 16) |
                                (static_cast<std::uint32_t>(g) << 8) | static_cast<std::uint32_t>(b);
    SDL_UpdateTexture(texture, nullptr, &pixel, sizeof(pixel));
    SDL_SetTextureBlendMode(texture, SDL_BLENDMODE_BLEND);
    store_texture(key, texture);
    return texture;
}

} // namespace d2df::render
