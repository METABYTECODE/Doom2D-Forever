#pragma once

#include <d2df/resources/asset_database.hpp>

#include <SDL.h>

#include <string>
#include <string_view>
#include <unordered_map>

namespace d2df::render {

class TextureCache {
public:
    TextureCache(SDL_Renderer* renderer, const d2df::resources::AssetDatabase& assets);
    ~TextureCache();

    TextureCache(const TextureCache&) = delete;
    TextureCache& operator=(const TextureCache&) = delete;

    [[nodiscard]] SDL_Texture* get(std::string_view asset_id);
    [[nodiscard]] SDL_Texture* builtin(std::string_view ref);
    [[nodiscard]] std::size_t cached_count() const { return textures_.size(); }
    [[nodiscard]] bool query_size(SDL_Texture* texture, int& width, int& height) const;

private:
    SDL_Texture* load_png(std::string_view asset_id);
    SDL_Texture* make_solid(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::string_view key);
    void store_texture(std::string_view key, SDL_Texture* texture);

    SDL_Renderer* renderer_;
    const d2df::resources::AssetDatabase& assets_;
    struct TextureEntry {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };
    std::unordered_map<std::string, TextureEntry> textures_;
};

} // namespace d2df::render
