#pragma once

#include <SDL_ttf.h>

#include <filesystem>
#include <string>
#include <string_view>
#include <unordered_map>

struct SDL_Renderer;

namespace d2df::render {

class TextRenderer {
public:
    TextRenderer();
    ~TextRenderer();

    TextRenderer(const TextRenderer&) = delete;
    TextRenderer& operator=(const TextRenderer&) = delete;

    bool init(const std::filesystem::path& content_root);
    void draw(SDL_Renderer* renderer, std::string_view text, int x, int y,
              std::uint8_t r = 220, std::uint8_t g = 220, std::uint8_t b = 230);
    [[nodiscard]] int measure_width(std::string_view text) const;
    void draw_right(SDL_Renderer* renderer, std::string_view text, int right_x, int y,
                    std::uint8_t r = 220, std::uint8_t g = 220, std::uint8_t b = 230);

private:
    struct TextCacheEntry {
        SDL_Texture* texture = nullptr;
        int width = 0;
        int height = 0;
    };

    [[nodiscard]] std::string cache_key(std::string_view text, std::uint8_t r, std::uint8_t g,
                                        std::uint8_t b) const;
    void clear_cache();
    [[nodiscard]] const TextCacheEntry* find_cached(SDL_Renderer* renderer, std::string_view text,
                                                    std::uint8_t r, std::uint8_t g,
                                                    std::uint8_t b) const;
    const TextCacheEntry& cache_text(SDL_Renderer* renderer, std::string_view text,
                                     std::uint8_t r, std::uint8_t g, std::uint8_t b);

    TTF_Font* font_ = nullptr;
    SDL_Renderer* cache_renderer_ = nullptr;
    std::unordered_map<std::string, TextCacheEntry> cache_;
};

} // namespace d2df::render
