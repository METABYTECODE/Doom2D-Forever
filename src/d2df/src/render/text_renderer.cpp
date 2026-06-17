#include <d2df/render/text_renderer.hpp>

#include <SDL.h>
#include <spdlog/spdlog.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace d2df::render {

TextRenderer::TextRenderer() {
    if (TTF_Init() != 0) {
        spdlog::warn("TTF_Init failed: {}", TTF_GetError());
    }
}

TextRenderer::~TextRenderer() {
    if (font_ != nullptr) {
        TTF_CloseFont(font_);
        font_ = nullptr;
    }
    TTF_Quit();
}

bool TextRenderer::init(const std::filesystem::path& content_root) {
    const std::filesystem::path candidates[] = {
        content_root / "fonts" / "flexui" / "font_10.ttf",
        content_root / "fonts" / "font_10.ttf",
    };

    for (const auto& path : candidates) {
        if (!std::filesystem::exists(path)) {
            continue;
        }
        font_ = TTF_OpenFont(path.string().c_str(), 16);
        if (font_ != nullptr) {
            spdlog::info("HUD font: {}", path.generic_string());
            return true;
        }
    }

    spdlog::warn("HUD font not found under content/fonts (trying system fallback)");

#ifdef _WIN32
    const char* system_fonts[] = {"C:\\Windows\\Fonts\\consola.ttf", "C:\\Windows\\Fonts\\arial.ttf"};
    for (const char* path : system_fonts) {
        if (std::filesystem::exists(path)) {
            font_ = TTF_OpenFont(path, 15);
            if (font_ != nullptr) {
                spdlog::info("HUD font (system): {}", path);
                return true;
            }
        }
    }
#endif

    return false;
}

int TextRenderer::measure_width(std::string_view text) const {
    if (font_ == nullptr || text.empty()) {
        return 0;
    }

    int width = 0;
    int height = 0;
    if (TTF_SizeUTF8(font_, std::string(text).c_str(), &width, &height) != 0) {
        return 0;
    }
    return width;
}

void TextRenderer::draw_right(SDL_Renderer* renderer, std::string_view text, int right_x, int y,
                              std::uint8_t r, std::uint8_t g, std::uint8_t b) const {
    const int width = measure_width(text);
    draw(renderer, text, right_x - width, y, r, g, b);
}

void TextRenderer::draw(SDL_Renderer* renderer, std::string_view text, int x, int y,
                        std::uint8_t r, std::uint8_t g, std::uint8_t b) const {
    if (font_ == nullptr || text.empty()) {
        return;
    }

    const SDL_Color color{r, g, b, 255};
    SDL_Surface* surface = TTF_RenderUTF8_Blended(font_, std::string(text).c_str(), color);
    if (surface == nullptr) {
        return;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture == nullptr) {
        return;
    }

    int w = 0;
    int h = 0;
    SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);
    const SDL_Rect dst{x, y, w, h};
    SDL_RenderCopy(renderer, texture, nullptr, &dst);
    SDL_DestroyTexture(texture);
}

} // namespace d2df::render
