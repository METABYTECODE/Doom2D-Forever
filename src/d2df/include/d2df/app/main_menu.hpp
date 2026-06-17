#pragma once

#include <d2df/render/text_renderer.hpp>

#include <filesystem>
#include <string>
#include <vector>

struct SDL_Renderer;

namespace d2df {

class MainMenu {
public:
    explicit MainMenu(SDL_Renderer* renderer, const std::filesystem::path& content_root);

    void reset();
    void handle_key_down(int sym, SDL_Scancode scancode);
    void render(int viewport_w, int viewport_h);

    [[nodiscard]] bool quit_requested() const { return quit_requested_; }
    [[nodiscard]] bool consume_map_selection(std::filesystem::path& out_map_path);

private:
    enum class Screen {
        Root,
        MapSelect,
    };

    void activate_root_selection();
    void activate_map_selection();
    [[nodiscard]] std::filesystem::path default_new_game_map() const;
    [[nodiscard]] std::string map_list_label(const std::filesystem::path& path) const;
    void draw_menu_list(int viewport_w, int viewport_h, const char* title,
                        const std::vector<std::string>& labels, int selection, int scroll_offset,
                        int visible_count);

    SDL_Renderer* renderer_;
    std::filesystem::path content_root_;
    render::TextRenderer text_;
    std::vector<std::filesystem::path> map_list_;

    Screen screen_ = Screen::Root;
    int root_selection_ = 0;
    int map_selection_ = 0;
    int map_scroll_ = 0;
    bool quit_requested_ = false;
    bool map_selected_ = false;
    std::filesystem::path selected_map_;
};

} // namespace d2df
