#pragma once

#include <filesystem>
#include <memory>
#include <optional>

#include <rivet/api.hpp>
#include <rivet/game/level_scene.hpp>
#include <rivet/game/level/level_data.hpp>
#include <rivet/game/model/model_animator.hpp>
#include <rivet/game/model/model_types.hpp>
#include <rivet/game/tileset/tileset_catalog.hpp>
#include <rivet/game/resources/resource_pack.hpp>
#include <rivet/physics/fluid_grid.hpp>

namespace rivet::game {

class LevelGame final : public rivet::core::Game {
public:
    explicit LevelGame(std::filesystem::path level_path);

    void on_attach(rivet::core::GameContext& context) override;
    void on_detach(rivet::core::GameContext& context) override;
    void on_update(rivet::core::GameContext& context, float delta_time) override;
    void on_fixed_update(rivet::core::GameContext& context, float fixed_delta_time) override;
    void on_render(rivet::core::GameContext& context, float interpolation_alpha) override;

private:
    void sync_player_motion(const LevelScene& scene);
    void update_camera(
        rivet::core::GameContext& context,
        float player_center_x,
        float player_center_y) const;
    void draw_level_background(rivet::render::IRenderer& renderer, const LevelScene& scene) const;
    void draw_level_world(
        rivet::render::IRenderer& renderer,
        const level::LevelData& data,
        float animation_time,
        float player_pivot_x,
        float player_pivot_y,
        float player_collider_offset_x,
        float player_collider_offset_y,
        float player_collider_width,
        float player_collider_height,
        int player_z,
        bool draw_player);

    void update_patrols(rivet::ecs::World& world, const std::vector<rivet::ecs::Entity>& patrols);
    void update_player_animation(float delta_time, float velocity_x, float velocity_y);
    [[nodiscard]] int player_render_z() const;

    std::filesystem::path level_path_;
    level::LevelData level_;
    rivet::physics::FluidGrid fluids_;
    rivet::resources::TextureHandle background_texture_ = rivet::resources::kInvalidTexture;
    std::unique_ptr<tileset::TilesetCatalog> tilesets_;
    std::optional<resources::ResourcePack> resource_pack_;
    std::filesystem::path assets_root_;
    std::filesystem::path music_path_;
    std::filesystem::path jump_sfx_path_;
    float animation_time_ = 0.0f;

    std::optional<model::ModelDef> player_model_;
    model::ModelAnimator player_animator_;
    float player_velocity_x_ = 0.0f;
    float player_velocity_y_ = 0.0f;

    struct PlayerMotion {
        float prev_x = 0.0f;
        float prev_y = 0.0f;
        float curr_x = 0.0f;
        float curr_y = 0.0f;
    } player_motion_;

    bool player_grounded_ = false;
    float jump_buffer_time_ = 0.0f;
    float coyote_time_ = 0.0f;
};

} // namespace rivet::game
