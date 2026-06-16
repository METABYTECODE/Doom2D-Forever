#pragma once

#include <SDL_ttf.h>

#include <filesystem>
#include <string>
#include <string_view>

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
              std::uint8_t r = 220, std::uint8_t g = 220, std::uint8_t b = 230) const;

private:
    TTF_Font* font_ = nullptr;
};

} // namespace d2df::render
