#pragma once

#include <SDL_scancode.h>

#include <d2df/core/fixed_timestep.hpp>
#include <d2df/ecs/game_world.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/render/camera2d.hpp>
#include <d2df/render/map_renderer.hpp>
#include <d2df/render/text_renderer.hpp>
#include <d2df/render/texture_cache.hpp>
#include <d2df/resources/asset_database.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/trigger_system.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

struct SDL_Renderer;

namespace d2df {

class MapViewer {
public:
    MapViewer(SDL_Renderer* renderer, std::filesystem::path content_root,
              std::filesystem::path map_path);

    void handle_key_down(int sym, SDL_Scancode scancode);
    void handle_key_up(int sym);
    void handle_mouse_wheel(int y);
    void update(float frame_dt);
    void render(int viewport_w, int viewport_h);

    [[nodiscard]] const map::MapDocument& map() const { return map_; }

private:
    void load_map(const std::filesystem::path& map_path);
    void cycle_map(int direction);
    void fixed_update();
    void draw_sky(int viewport_w, int viewport_h);
    void draw_player(int viewport_w, int viewport_h);
    void draw_hud(int viewport_w, int viewport_h);
    void update_camera_follow();

    SDL_Renderer* renderer_;
    std::filesystem::path content_root_;
    d2df::resources::AssetDatabase assets_;
    map::MapDocument map_;
    render::Camera2D camera_;
    render::MapRenderer map_renderer_;
    render::TextRenderer text_;
    sim::MapCollision collision_;
    sim::TriggerSystem triggers_;
    ecs::GameWorld world_;
    FixedTimestep fixed_timestep_;
    std::unique_ptr<render::TextureCache> textures_;
    std::vector<std::filesystem::path> map_list_;
    std::size_t map_index_ = 0;

    bool free_camera_ = false;
    bool key_left_ = false;
    bool key_right_ = false;
    bool key_jump_ = false;
    bool key_use_ = false;
    bool key_use_edge_ = false;
    float camera_move_speed_ = 480.0f;
};

} // namespace d2df
