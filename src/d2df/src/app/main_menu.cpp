#include <d2df/app/main_menu.hpp>

#include <SDL.h>

#include <algorithm>

#include <d2df/map/map_catalog.hpp>

namespace d2df {
namespace {

constexpr int kRootItemCount = 3;
constexpr const char* kRootItems[] = {"New game", "Select map", "Quit"};
constexpr int kMapSelectVisible = 14;

} // namespace

MainMenu::MainMenu(SDL_Renderer* renderer, const std::filesystem::path& content_root)
    : renderer_(renderer)
    , content_root_(content_root) {
    text_.init(content_root);
    map_list_ = map::list_map_json_files(content_root_);
    reset();
}

void MainMenu::reset() {
    screen_ = Screen::Root;
    root_selection_ = 0;
    map_selection_ = 0;
    map_scroll_ = 0;
    quit_requested_ = false;
    map_selected_ = false;
    selected_map_.clear();
}

std::filesystem::path MainMenu::default_new_game_map() const {
    const std::filesystem::path preferred = content_root_ / "maps" / "doom2d" / "map01.json";
    if (std::filesystem::exists(preferred)) {
        return preferred;
    }
  if (!map_list_.empty()) {
        return map_list_.front();
    }
    return preferred;
}

std::string MainMenu::map_list_label(const std::filesystem::path& path) const {
    std::error_code ec;
    const auto rel = std::filesystem::relative(path, content_root_ / "maps", ec);
    if (!ec) {
        return rel.generic_string();
    }
    return path.filename().string();
}

void MainMenu::draw_menu_list(int viewport_w, int viewport_h, const char* title,
                              const std::vector<std::string>& labels, int selection,
                              int scroll_offset, int visible_count) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 190);
    const SDL_Rect overlay{0, 0, viewport_w, viewport_h};
    SDL_RenderFillRect(renderer_, &overlay);

    const int line_height = 22;
    const int visible = std::min(visible_count, static_cast<int>(labels.size()));
    const int panel_height = 36 + visible * line_height;
    const int panel_y = (viewport_h - panel_height) / 2;

    text_.draw(renderer_, title, viewport_w / 2 - text_.measure_width(title) / 2, panel_y, 255,
               180, 80);

    for (int row = 0; row < visible; ++row) {
        const int index = scroll_offset + row;
        if (index >= static_cast<int>(labels.size())) {
            break;
        }
        const bool selected = index == selection;
        const std::string prefix = selected ? "> " : "  ";
        const std::string label = prefix + labels[index];
        const int y = panel_y + 36 + row * line_height;
        if (selected) {
            text_.draw(renderer_, label, viewport_w / 2 - text_.measure_width(label) / 2, y, 255,
                       220, 120);
        } else {
            text_.draw(renderer_, label, viewport_w / 2 - text_.measure_width(label) / 2, y, 200,
                       200, 210);
        }
    }

    const char* hint = "Up/Down  Enter  Esc";
    text_.draw(renderer_, hint, viewport_w / 2 - text_.measure_width(hint) / 2,
               panel_y + panel_height + 8, 150, 150, 160);
}

void MainMenu::activate_root_selection() {
    switch (root_selection_) {
    case 0:
        selected_map_ = default_new_game_map();
        map_selected_ = true;
        break;
    case 1:
        if (map_list_.empty()) {
            break;
        }
        screen_ = Screen::MapSelect;
        map_selection_ = 0;
        map_scroll_ = 0;
        break;
    case 2:
        quit_requested_ = true;
        break;
    default:
        break;
    }
}

void MainMenu::activate_map_selection() {
    if (map_selection_ < 0 || map_selection_ >= static_cast<int>(map_list_.size())) {
        return;
    }
    selected_map_ = map_list_[map_selection_];
    map_selected_ = true;
}

void MainMenu::handle_key_down(int sym, SDL_Scancode scancode) {
    switch (screen_) {
    case Screen::Root:
        switch (scancode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            root_selection_ = (root_selection_ + kRootItemCount - 1) % kRootItemCount;
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            root_selection_ = (root_selection_ + 1) % kRootItemCount;
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_SPACE:
            activate_root_selection();
            break;
        case SDL_SCANCODE_1:
            root_selection_ = 0;
            activate_root_selection();
            break;
        case SDL_SCANCODE_2:
            root_selection_ = 1;
            activate_root_selection();
            break;
        case SDL_SCANCODE_3:
            root_selection_ = 2;
            activate_root_selection();
            break;
        default:
            break;
        }
        break;
    case Screen::MapSelect:
        if (map_list_.empty()) {
            screen_ = Screen::Root;
            return;
        }
        switch (scancode) {
        case SDL_SCANCODE_UP:
        case SDL_SCANCODE_W:
            map_selection_ = (map_selection_ + static_cast<int>(map_list_.size()) - 1) %
                             static_cast<int>(map_list_.size());
            if (map_selection_ < map_scroll_) {
                map_scroll_ = map_selection_;
            } else if (map_selection_ >= map_scroll_ + kMapSelectVisible) {
                map_scroll_ = map_selection_ - kMapSelectVisible + 1;
            }
            break;
        case SDL_SCANCODE_DOWN:
        case SDL_SCANCODE_S:
            map_selection_ = (map_selection_ + 1) % static_cast<int>(map_list_.size());
            if (map_selection_ < map_scroll_) {
                map_scroll_ = map_selection_;
            } else if (map_selection_ >= map_scroll_ + kMapSelectVisible) {
                map_scroll_ = map_selection_ - kMapSelectVisible + 1;
            }
            break;
        case SDL_SCANCODE_PAGEUP:
            map_selection_ =
                std::max(0, map_selection_ - kMapSelectVisible);
            map_scroll_ = std::max(0, map_scroll_ - kMapSelectVisible);
            break;
        case SDL_SCANCODE_PAGEDOWN:
            map_selection_ =
                std::min(static_cast<int>(map_list_.size()) - 1,
                         map_selection_ + kMapSelectVisible);
            map_scroll_ = std::min(
                std::max(0, static_cast<int>(map_list_.size()) - kMapSelectVisible),
                map_scroll_ + kMapSelectVisible);
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_SPACE:
            activate_map_selection();
            break;
        case SDL_SCANCODE_ESCAPE:
        case SDL_SCANCODE_BACKSPACE:
            screen_ = Screen::Root;
            break;
        default:
            break;
        }
        break;
    }

    if (sym == SDLK_ESCAPE && screen_ == Screen::Root) {
        quit_requested_ = true;
    }
}

bool MainMenu::consume_map_selection(std::filesystem::path& out_map_path) {
    if (!map_selected_) {
        return false;
    }
    out_map_path = selected_map_;
    map_selected_ = false;
    selected_map_.clear();
    return true;
}

void MainMenu::render(int viewport_w, int viewport_h) {
    SDL_SetRenderDrawColor(renderer_, 20, 15, 26, 255);
    SDL_RenderClear(renderer_);

    const char* subtitle = "Doom2D Forever";
    text_.draw(renderer_, subtitle, viewport_w / 2 - text_.measure_width(subtitle) / 2, 48, 180,
               180, 200);

    switch (screen_) {
    case Screen::Root: {
        std::vector<std::string> labels;
        for (int i = 0; i < kRootItemCount; ++i) {
            labels.push_back(std::to_string(i + 1) + ". " + kRootItems[i]);
        }
        draw_menu_list(viewport_w, viewport_h, "MAIN MENU", labels, root_selection_, 0,
                       kRootItemCount);
        break;
    }
    case Screen::MapSelect: {
        std::vector<std::string> map_labels;
        for (const auto& path : map_list_) {
            map_labels.push_back(map_list_label(path));
        }
        const std::string title =
            "SELECT MAP (" + std::to_string(map_list_.size()) + ")";
        draw_menu_list(viewport_w, viewport_h, title.c_str(), map_labels, map_selection_,
                       map_scroll_, kMapSelectVisible);
        break;
    }
    }
}

} // namespace d2df
