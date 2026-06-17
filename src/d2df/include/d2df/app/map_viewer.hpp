#pragma once

#include <SDL_scancode.h>

#include <d2df/audio/player_continuous_sfx.hpp>
#include <d2df/audio/sound_system.hpp>
#include <d2df/core/event_bus.hpp>
#include <d2df/core/fixed_timestep.hpp>
#include <d2df/core/game_events.hpp>
#include <d2df/core/resource_audit.hpp>
#include <d2df/ecs/game_world.hpp>
#include <d2df/map/map_document.hpp>
#include <d2df/render/camera2d.hpp>
#include <d2df/render/map_renderer.hpp>
#include <d2df/render/map_render_index.hpp>
#include <d2df/render/text_renderer.hpp>
#include <d2df/render/texture_cache.hpp>
#include <d2df/resources/asset_database.hpp>
#include <d2df/sim/map_collision.hpp>
#include <d2df/sim/effect_types.hpp>
#include <d2df/sim/trigger_system.hpp>
#include <d2df/sim/weapon_types.hpp>

#include <filesystem>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#if D2DF_DEBUG_UI
namespace d2df::debug {
class DebugUi;
} // namespace d2df::debug
#endif

struct SDL_Renderer;

namespace d2df {

class EventBus;

struct WorldEffect {
    sim::ExplosionSprite sprite{};
    float x = 0.0f;
    float y = 0.0f;
    int anim_tick = 0;
    int duration_ticks = 0;
    std::uint8_t alpha = 255;
    bool bubble = false;
};

class MapViewer {
public:
    MapViewer(SDL_Renderer* renderer, std::filesystem::path content_root,
              std::filesystem::path map_path, EventBus* events = nullptr);
    ~MapViewer();

    void handle_key_down(int sym, SDL_Scancode scancode);
    void handle_key_up(int sym);
    void handle_mouse_wheel(int y);
    void update(float frame_dt);
    void render(int viewport_w, int viewport_h);

    void toggle_pause();
    [[nodiscard]] bool is_paused() const { return paused_; }
    [[nodiscard]] bool consume_quit_request();
    [[nodiscard]] bool consume_return_to_main_menu_request();

#if D2DF_DEBUG_UI
    void set_debug_ui(debug::DebugUi* debug_ui) { debug_ui_ = debug_ui; }
    void draw_debug_overlays(int viewport_w, int viewport_h);
#endif

    [[nodiscard]] const map::MapDocument& map() const { return map_; }
    [[nodiscard]] const render::Camera2D& camera() const { return camera_; }
    [[nodiscard]] core::ResourceSnapshot build_resource_snapshot() const;

private:
    void load_map(const std::filesystem::path& map_path);
    void cycle_map(int direction);
    void fixed_update();
    void draw_sky(int viewport_w, int viewport_h);
    void draw_player(int viewport_w, int viewport_h);
    void draw_player_corpses(int viewport_w, int viewport_h);
    void draw_targets(int viewport_w, int viewport_h);
    void draw_items(int viewport_w, int viewport_h, bool monster_drops_only = false);
    void draw_projectiles(int viewport_w, int viewport_h);
    void draw_effects(int viewport_w, int viewport_h);
    void tick_effects();
    void spawn_explosion_effect(const events::WorldExplosion& event);
    void spawn_smoke_effect(const events::WorldSmokePuff& event);
    void spawn_bubble_effect(const events::WorldBubblePuff& event);
    void draw_hud(int viewport_w, int viewport_h);
    void draw_player_overlays(int viewport_w, int viewport_h);
    void draw_pause_menu(int viewport_w, int viewport_h);
    void draw_perf_overlay(int viewport_w, int viewport_h);
    void update_camera_follow(float render_alpha = 1.0f);
    void clear_input_state();
    void handle_pause_key_down(int sym, SDL_Scancode scancode);
    void activate_pause_selection();
    void save_quicksave();
    void load_quicksave();
    [[nodiscard]] std::filesystem::path quicksave_path() const;
    [[nodiscard]] std::string current_map_rel_path() const;

    SDL_Renderer* renderer_;
    std::filesystem::path content_root_;
    std::filesystem::path current_map_path_;
    d2df::resources::AssetDatabase assets_;
    map::MapDocument map_;
    render::MapRenderIndex map_render_index_;
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
    bool key_aim_up_ = false;
    bool key_aim_down_ = false;
    bool key_weapon_prev_ = false;
    int weapon_select_request_ = -1;
    bool paused_ = false;
    int pause_selection_ = 0;
    bool quit_requested_ = false;
    bool return_to_main_menu_requested_ = false;
    std::string save_status_message_;
    int save_status_ticks_ = 0;
    float camera_move_speed_ = 480.0f;
    EventBus* events_ = nullptr;
    audio::SoundSystem sound_;
    audio::PlayerContinuousSfx continuous_sfx_;
    std::vector<WorldEffect> effects_;
#if D2DF_DEBUG_UI
    debug::DebugUi* debug_ui_ = nullptr;
#endif
};

} // namespace d2df
